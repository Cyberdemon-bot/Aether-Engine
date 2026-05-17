#include "aepch.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Scripting/ScriptGlue.h"
#include "Aether/Core/Log.h"
#include "Aether/Core/KeyCodes.h"
#include "Aether/Physics/PhysicsAPI.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Assets/Script.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Core/JobSystem.h"

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
       lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::coroutine);
       instance.m_Instances.Init();
       instance.m_Sources.Init();
       instance.m_DestroyQueue.reserve(32);
       instance.m_EventManager.emplace(instance.LuaState.lua);
       RegisterBinding();
       AE_CORE_INFO("ScriptEngine initialized with {0}", LUA_VERSION);
    }

    void ScriptEngine::Shutdown()
    {
        auto& instance = GetInstance();
        instance.m_Instances.Shutdown();
        instance.m_Sources.Shutdown();
        instance.m_DestroyQueue.clear();
    }

    void ScriptEngine::RegisterBinding()
    {
        BindEnum<Key::KeyCode>("Key");
        BindEnum<Mouse::MouseCode>("Mouse");
        BindEnum<CollisionType>("CollisionType");
        BindEnum<LightType>("LightType"); 
        BindEnum<SceneCamera::ProjectionType>("CameraProjection");
        BindType<Vec3Binding>("Math");
        BindType<QuatBinding>("Math");
        BindModule<MathBinding>("Math");
        BindModule<InputBinding>("Input"); 
        BindType<TransformComponentBinding>();
        BindType<LightParamBinding>();   
        BindType<SceneCameraBinding>();
        BindType<ScriptSelfBinding>();
        BindType<SceneBinding>();
        BindType<EventManagerBinding>();
        BindType<CollisionBinding>();
        BindType<RaycastHitBinding>();
        BindType<PhysicsBinding>();

        auto& instance = GetInstance();
        auto& lua = instance.LuaState.lua;
        lua["Native"] = lua.create_table();
        sol::table native = lua["Native"];
        native.set_function("Async", [](std::string name, sol::table args, sol::protected_function callback)
        {
            auto& engine = GetInstance();
            auto it = std::find_if(engine.m_NativeFuncs.begin(), engine.m_NativeFuncs.end(), [name](const NativeFunc& func) { return func.name == name; });
            if (it == engine.m_NativeFuncs.end())
            {
                AE_CORE_WARN("[ScriptEngine] Native.Async: func '{0}' not found", name);
                return;
            }
            auto delegate = it->native;
            ScriptArgs scriptArgs;
            for (size_t i = 1; i <= args.size(); i++)
                scriptArgs.Pushback(FromSolObject(args[i]));

            JobSystem::SubmitJob([delegate, scriptArgs, callback, &engine]()
            {
                ScriptValue result = delegate(scriptArgs);
                std::lock_guard<std::mutex> lock(engine.m_PendingMutex);
                engine.m_PendingCallbacks.push_back({callback, result});
            });
        });
    }

    Handle<ScriptInstance> ScriptEngine::CreateInstance(Scene* scene, Entity entity, Handle<Bytecode> bh)
    {
        auto& instance = GetInstance(); 
        auto it = instance.m_Sources.GetResource(bh);
        if (it == nullptr) return Handle<ScriptInstance>::MakeInvalid();

        sol::bytecode bytecode = it->bytecode;
        auto env_handle = instance.LuaState.CreateEnvironment();
        sol::environment env = *instance.LuaState.env_pool.GetResource(env_handle);
        auto& lua = instance.LuaState.lua;
        lua.script(bytecode.as_string_view(), env);

        auto handle = instance.m_Instances.CreateResource();
        auto slot = instance.m_Instances.GetResource(handle);

        ScriptSelf self{ scene, entity, slot};
        SceneContext sceneCtx{ scene };
        EventContext eventCtx{ handle, &instance.m_EventManager.value() };
        PhysicsContext physicsCtx{ scene, entity };
        env["self"] = self;
        env["scene"] = sceneCtx;
        env["event"] = eventCtx;
        env["physics"] = physicsCtx;

        slot->env_hanle = env_handle;
        slot->ctx = scene;
        slot->code_handle = bh;
        MarkExecOrderChanged();
        return handle; 
    }

    void ScriptEngine::StartInstance(Handle<ScriptInstance> handle)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;
        CallMethod(*slot, "OnStart");
    }

    Handle<Bytecode> ScriptEngine::LoadScript(const std::string& path, bool saveRaw)
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
            return Handle<Bytecode>::MakeInvalid();
        }

        sol::bytecode bytecode = res.get<sol::function>().dump();
        if (!saveRaw) source.clear();
        auto handle = instance.m_Sources.SaveResource({bytecode, source});
        return handle;
    }

    Handle<Bytecode> ScriptEngine::LoadScriptSource(const std::string& source)
    {
        auto& instance = GetInstance();
        auto& lua = instance.LuaState.lua;

        sol::load_result res = lua.load(source);
        if (!res.valid())
        {
            sol::error err = res;
            AE_CORE_ERROR("[Script] Compile error: {0}", err.what());
            return Handle<Bytecode>::MakeInvalid();
        }

        sol::bytecode bytecode = res.get<sol::function>().dump();
        return instance.m_Sources.SaveResource({bytecode, source});
    }

    std::string ScriptEngine::GetRaw(Handle<Bytecode> handle)
    {
        auto& instance = GetInstance();
        auto source = instance.m_Sources.GetResource(handle);
        if (source == nullptr) return {};
        return source->rawcode;
    }

    std::string ScriptEngine::GetRaw(Handle<ScriptInstance> handle)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return {};
        return GetRaw(slot->code_handle);
    }

    void ScriptEngine::DestroyInstance(Handle<ScriptInstance> handle)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;
        CallMethod(*slot, "OnDestroy");

        instance.LuaState.RemoveEnvironment(slot->env_hanle);
        instance.m_Instances.DestroyResource(handle);
        instance.m_EventManager->RemoveListener(handle);
        MarkExecOrderChanged();
    }

    void ScriptEngine::UpdateInstance(Handle<ScriptInstance> handle, Timestep ts)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;
        if (slot->has_error) return;

        CallMethod(*slot, "OnUpdate", (float)ts);
    }

    void ScriptEngine::OnInstanceCollision(Handle<ScriptInstance> handle, CollisionData data)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;
        if (slot->has_error) return;

        CallMethod(*slot, "OnCollision", data);
    }

    void ScriptEngine::SetActiveStage(Handle<ScriptInstance> handle, bool active)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return;
        slot->is_active = active;
    }

    bool ScriptEngine::GetActiveStage(Handle<ScriptInstance> handle)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return false;
        return slot->is_active;
    }

    void ScriptEngine::FlushEvent()
    {
        auto& instance = GetInstance();
        auto& lua = instance.LuaState.lua;
        std::vector<PendingCallback> pending;
        {
            std::lock_guard<std::mutex> lock(instance.m_PendingMutex);
            std::swap(pending, instance.m_PendingCallbacks);
        }
        for (auto& p : pending)
        {
            auto result = p.callback(ToSolObject(lua, p.result));
            if (!result.valid())
            {
                sol::error err = result;
                AE_CORE_ERROR("[ScriptEngine] Async callback error: {0}", err.what());
            }
        }
        instance.m_EventManager->Flush();

        for (auto& [e, handle] : instance.m_DestroyQueue)
        {
            auto* it = instance.m_Instances.GetResource(handle);
            if (it == nullptr) continue;

            it->ctx->DestroyEntity(e);
            DestroyInstance(handle);
        }
        instance.m_DestroyQueue.clear();
    }

    int ScriptEngine::GetExecOrder(Handle<ScriptInstance> handle)
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

    sol::object ScriptEngine::CallSafeInstanceAPI(Handle<ScriptInstance> handle, const std::string& name, const std::vector<sol::object>& args)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return sol::lua_nil;

        auto it = std::find_if(slot->exposed_funcs.begin(), slot->exposed_funcs.end(), [name](const Exposed& data) { return data.name == name; });
        if (it == slot->exposed_funcs.end()) return sol::lua_nil;

        sol::protected_function_result result = it->func(sol::as_args(args));
        if (!result.valid())
        {
            sol::error err = result;
            AE_CORE_ERROR("[Script] CallSafeInstanceAPI error in '{0}': {1}", name, err.what());
            return sol::lua_nil;
        }
        return result;
    }

    sol::object ScriptEngine::CallDirectInstanceAPI(Handle<ScriptInstance> handle, const std::string& name, const std::vector<sol::object>& args)
    {
        auto& instance = GetInstance();
        auto slot = instance.m_Instances.GetResource(handle);
        if (slot == nullptr) return sol::lua_nil;

        auto env = instance.LuaState.env_pool.GetResource(slot->env_hanle);
        if (env == nullptr) return sol::lua_nil;

        sol::protected_function func = (*env)[name];
        if (!func.valid()) return sol::lua_nil;

        sol::protected_function_result result = func(sol::as_args(args));
        if (!result.valid())
        {
            sol::error err = result;
            AE_CORE_ERROR("[Script] CallInstanceAPI error in '{0}': {1}", name, err.what());
            return sol::lua_nil;
        }
        return result;
    }

    Handle<ScriptCallback> ScriptEngine::AddListener(const std::string& event_name, Delegate<void(const ScriptArgs& args)> callback)
    {
        auto& instance = GetInstance();
        return instance.m_EventManager->AddNativeListener(event_name, callback);
    }

    void ScriptEngine::RemoveListener(Handle<ScriptCallback> handle, const std::string& event_name)
    {
        auto& instance = GetInstance();
        instance.m_EventManager->RemoveNativeListener(handle, event_name);
    }

    void ScriptEngine::ImportNativeFunc(const std::string& name, Delegate<ScriptValue(const ScriptArgs&)> func)
    {
        auto& instance = GetInstance();
        auto& lua = instance.LuaState.lua;
        sol::table native = lua["Native"];
        instance.m_NativeFuncs.push_back({name, func});
        native.set_function(name, [func, &lua](sol::variadic_args va) -> sol::object
        {
            ScriptArgs args;
            for (const auto& v : va)
                args.Pushback(FromSolObject(v));
            
            ScriptValue result = func(args);
            return ToSolObject(lua, result);
        });
    }
}