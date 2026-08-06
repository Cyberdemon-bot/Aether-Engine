#include "aepch.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Scripting/ScriptGlue.h"
#include "Aether/Core/Log.h"
#include "Aether/Physics/PhysicsAPI.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Assets/Script.h"

namespace Aether {

    sol::meta_function ScriptEngine::OpNameToMeta(std::string_view name)
    {
        if (name == "ADD") return sol::meta_function::addition;
        if (name == "SUB") return sol::meta_function::subtraction;
        if (name == "MUL") return sol::meta_function::multiplication;
        if (name == "DIV") return sol::meta_function::division;
        if (name == "EQ")  return sol::meta_function::equal_to;
        if (name == "LT")  return sol::meta_function::less_than;
        if (name == "LE")  return sol::meta_function::less_than_or_equal_to;
        if (name == "MOD") return sol::meta_function::modulus;
        if (name == "UNM") return sol::meta_function::unary_minus;
        AE_CORE_WARN("[ScriptEngine] Unknown op name: {0}", name);
        return sol::meta_function::addition;
    }

    void ScriptEngine::Init()
    {   
        auto& lua = LuaState.lua;
        lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::coroutine);
        lua_gc(lua, LUA_GCGEN, 0, 0);
        m_Instances.Init();
        m_Sources.Init();
        m_PromiseManager.Init();
        m_EventManager.Init();
        m_CoroutineManager.Init(LuaState.lua);
        m_DestroyQueue.reserve(32);
        RegisterBinding();
        AE_CORE_INFO("ScriptEngine initialized with {0}/Sol {1}", LUA_VERSION, SOL_VERSION_MAJOR);
    }

    void ScriptEngine::Shutdown()
    {
        m_CoroutineManager.Shutdown(); 
        m_PromiseManager.Shutdown();
        m_Instances.Shutdown();
        m_Sources.Shutdown();
        m_EventManager.Shutdown();
        m_DestroyQueue.clear();
    }

    void ScriptEngine::Update(Timestep ts)
    {
        FlushEvent();
        m_CoroutineManager.Update(ts);
        m_PromiseManager.Flush();
    }

    Handle<Bytecode> ScriptEngine::LoadScript(const std::string& source)
    {
        auto& lua = LuaState.lua;

        sol::load_result res = lua.load(source);
        if (!res.valid())
        {
            sol::error err = res;
            AE_CORE_ERROR("[Script] Compile error: {0}", err.what());
            return Handle<Bytecode>::MakeInvalid();
        }

        sol::bytecode bytecode = res.get<sol::function>().dump();
        return m_Sources.SaveResource({bytecode});
    }

    Handle<ScriptInstance> ScriptEngine::CreateInstance(Scene* scene, Entity entity, Handle<Bytecode> bh)
    { 
        auto it = m_Sources.GetResource(bh);
        if (it == nullptr) return Handle<ScriptInstance>::MakeInvalid();

        sol::bytecode bytecode = it->bytecode;
        auto env_handle = LuaState.CreateEnvironment();
        sol::environment env = *LuaState.env_pool.GetResource(env_handle);
        auto& lua = LuaState.lua;
        lua.script(bytecode.as_string_view(), env);

        auto handle = m_Instances.CreateResource();
        auto slot = m_Instances.GetResource(handle);

        ScriptSelf self{ scene, entity, slot};
        SceneContext sceneCtx{ scene };
        EventContext eventCtx{ handle, &m_EventManager, &m_PromiseManager };
        PhysicsContext physicsCtx{ scene, entity };
        CoroutineContext coroutineCtx{ handle, &m_CoroutineManager, &m_PromiseManager };
        PromiseContext promiseCtx{ &m_PromiseManager };
        JobContext jobCtx{ &m_NativeFuncs, &m_PromiseManager };

        env["self"] = self;
        env["Scene"] = sceneCtx;
        env["Event"] = eventCtx;
        env["Physics"] = physicsCtx;
        env["Coroutine"] = coroutineCtx;
        env["Promise"] = promiseCtx;
        env["Job"] = jobCtx;

        slot->env_handle = env_handle;
        slot->ctx = scene;
        slot->code_handle = bh;
        IsExecChanged = true;
        return handle; 
    }

    void ScriptEngine::DestroyInstance(Handle<ScriptInstance> handle)
    {
        m_DestroyQueue.push_back({handle});
        CallDirectInstanceAPI(handle, "OnDestroy");
    }

    void ScriptEngine::StartInstance(Handle<ScriptInstance> handle)
    {
        CallDirectInstanceAPI(handle, "OnStart");
    }

    void ScriptEngine::OnInstanceCollision(Handle<ScriptInstance> handle, CollisionData data)
    {
        CallDirectInstanceAPI(handle, "OnCollision", data);
    }

    void ScriptEngine::UpdateInstance(Handle<ScriptInstance> handle, Timestep ts)
    {
        CallDirectInstanceAPI(handle, "OnUpdate", (float)ts);
    }

    void ScriptEngine::FlushEvent()
    {
        auto& lua = LuaState.lua;
        m_EventManager.Flush();

        for (auto& handle : m_DestroyQueue)
        {
            auto slot = m_Instances.GetResource(handle);
            if (slot == nullptr) continue;

            LuaState.RemoveEnvironment(slot->env_handle);
            m_Instances.DestroyResource(handle);
            IsExecChanged = true;
        }
        m_DestroyQueue.clear();
    }

    int ScriptEngine::GetExecOrder(Handle<ScriptInstance> handle)
    {
        auto slot = m_Instances.GetResource(handle);
        if (slot == nullptr) return -1;
        return slot->exec_order;
    }

    bool ScriptEngine::IsExecOrderChanged() 
    { 
        if(IsExecChanged)
        {
            IsExecChanged = false;
            return true;
        }
        return false;
    }
}