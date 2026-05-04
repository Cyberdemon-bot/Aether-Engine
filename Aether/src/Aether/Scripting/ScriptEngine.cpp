#include "aepch.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Scripting/ScriptGlue.h"
#include "Aether/Core/Log.h"
#include "Aether/Core/KeyCodes.h"
#include "Aether/Physics/PhysicsAPI.h"

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

    ScriptEngine& ScriptEngine::GetInstance()
    {
        static ScriptEngine instance;
        return instance;
    }

    void ScriptEngine::Init()
    {   
       auto& instance = GetInstance();
       auto& lua = instance.LuaState.lua;
       lua.open_libraries(sol::lib::base, sol::lib::math);
       instance.m_Instances.Init();
       instance.m_Sources.Init();
       instance.m_DestroyQueue.reserve(32);
       instance.m_CreateQueue.reserve(32);
       instance.m_EventManager.emplace(instance.LuaState.lua);
       RegisterTypes();
       AE_CORE_INFO("ScriptEngine initialized with {0}", LUA_VERSION);
    }

    void ScriptEngine::Shutdown()
    {
        auto& instance = GetInstance();
        instance.m_Instances.Shutdown();
        instance.m_Sources.Shutdown();
        instance.m_DestroyQueue.clear();
        instance.m_CreateQueue.clear();
    }

    void ScriptEngine::RegisterTypes()
    {
        BindEnum<Key::KeyCode>("Key");
        BindEnum<Mouse::MouseCode>("Mouse");
        BindEnum<CollisionType>("CollisionType");
        BindType<Vec3Binding>("Math");
        BindType<QuatBinding>("Math");
        BindModule<MathBinding>("Math");
        BindModule<InputBinding>("Input"); 
        BindType<TransformComponentBinding>();
        BindType<ScriptSelfBinding>();
        BindType<SceneBinding>();
        BindType<EventManagerBinding>();
        BindType<CollisionBinding>();
    }

    Handle<ScriptTag> ScriptEngine::CreateInstance(Scene* scene, Entity entity, Handle<BytecodeTag> bh)
    {
        auto& instance = GetInstance(); 
        auto it = instance.m_Sources.GetResource(bh);
        if (it == nullptr) return Handle<ScriptTag>::MakeInvalid();

        sol::bytecode bytecode = *it;
        auto env_handle = instance.LuaState.CreateEnvironment();
        sol::environment env = *instance.LuaState.env_pool.GetResource(env_handle);
        auto& lua = instance.LuaState.lua;
        lua.script(bytecode.as_string_view(), env);

        auto handle = instance.m_Instances.CreateResource();
        auto slot = instance.m_Instances.GetResource(handle);

        ScriptSelf self{ scene, entity, slot};
        SceneContext sceneCtx{ scene };
        EventContext eventCtx{ handle, &instance.m_EventManager.value() };
        env["self"] = self;
        env["scene"] = sceneCtx;
        env["event"] = eventCtx;

        slot->env_hanle = env_handle;
        slot->ctx = scene;
        MarkExecOrderChanged();
        return handle; 
    }

    void ScriptEngine::StartInstance(Handle<ScriptTag> handle)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;
        CallMethod(*slot, "OnStart");
    }

    Handle<BytecodeTag> ScriptEngine::LoadScript(const std::string& path)
    {
        auto& instance = GetInstance();
        auto& lua = instance.LuaState.lua;

        std::ifstream file(path);
        std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        sol::load_result res = lua.load(source);
        if (!res.valid())
        {
            sol::error err = res;
            AE_CORE_ERROR("[Script] Compile error: {0}", err.what());
            return Handle<BytecodeTag>::MakeInvalid();
        }

        sol::bytecode bytecode = res.get<sol::function>().dump();
        return instance.m_Sources.SaveResource(bytecode);
    }

    void ScriptEngine::DestroyInstance(Handle<ScriptTag> handle)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;
        CallMethod(*slot, "OnDestroy");

        instance.LuaState.RemoveEnvironment(slot->env_hanle);
        instance.m_Instances.DestroyResource(handle);
        MarkExecOrderChanged();
    }

    void ScriptEngine::UpdateInstance(Handle<ScriptTag> handle, Timestep ts)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;
        if (slot->has_error) return;

        CallMethod(*slot, "OnUpdate", (float)ts);
    }

    void ScriptEngine::OnInstanceCollision(Handle<ScriptTag> handle, CollisionData data)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;
        if (slot->has_error) return;

        CallMethod(*slot, "OnCollision", data);
    }

    void ScriptEngine::SetActiveStage(Handle<ScriptTag> handle, bool active)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;
        slot->is_active = active;
    }

    bool ScriptEngine::GetActiveStage(Handle<ScriptTag> handle)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return false;
        return slot->is_active;
    }

    void ScriptEngine::FlushEvent()
    {
        auto& instance = GetInstance();
        instance.m_EventManager->Flush();

        for (auto& [e, handle] : instance.m_DestroyQueue)
        {
            auto* it = instance.m_Instances.GetResource(handle);
            if (it == nullptr) continue;

            it->ctx->DestroyEntity(e);
            DestroyInstance(handle);
        }
        instance.m_DestroyQueue.clear();

        for (auto& [e, handle] : instance.m_CreateQueue)
        {
            auto* it = instance.m_Instances.GetResource(handle);
            if (it == nullptr) continue;

            it->ctx->AddComponent<ScriptComponent>(e, handle);
            StartInstance(handle);
        }
        instance.m_CreateQueue.clear();

    }

    int ScriptEngine::GetExecOrder(Handle<ScriptTag> handle)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return -1;
        return slot->exec_order;
    }

    bool ScriptEngine::IsExecOrderChanged() 
    { 
        auto& instance = GetInstance();
        if(instance.IsExecChanged)
        {
            instance.IsExecChanged = false;
            return true;
        }
        return false;
    }
}