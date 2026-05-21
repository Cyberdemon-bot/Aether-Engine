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
       instance.m_Coroutines.Init();
       instance.m_DestroyQueue.reserve(32);
       instance.m_EventManager.emplace(instance.LuaState.lua);
       RegisterBinding();
       AE_CORE_INFO("ScriptEngine initialized with {0}/Sol {1}", LUA_VERSION, SOL_VERSION_MAJOR);
    }

    void ScriptEngine::Shutdown()
    {
        auto& instance = GetInstance();
        instance.m_Instances.Shutdown();
        instance.m_Sources.Shutdown();
        instance.m_Coroutines.Shutdown();
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
        BindType<AsyncBinding>();
    }

    Handle<Bytecode> ScriptEngine::LoadScript(const std::string& path)
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
        auto handle = instance.m_Sources.SaveResource({bytecode});
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
        return instance.m_Sources.SaveResource({bytecode});
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
        AsyncContext asyncCtx{ handle };
        env["self"] = self;
        env["Scene"] = sceneCtx;
        env["Event"] = eventCtx;
        env["Physics"] = physicsCtx;
        env["Async"] = asyncCtx;

        slot->env_hanle = env_handle;
        slot->ctx = scene;
        slot->code_handle = bh;
        instance.IsExecChanged = true;
        return handle; 
    }

    void ScriptEngine::DestroyInstance(Handle<ScriptInstance> handle)
    {
        auto& instance = GetInstance();
        instance.m_DestroyQueue.push_back({handle});
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
        instance.m_EventManager->Flush();

        for (auto& handle : instance.m_DestroyQueue)
        {
            auto slot = instance.m_Instances.GetResource(handle);
            if (slot == nullptr) return;

            instance.LuaState.RemoveEnvironment(slot->env_hanle);
            instance.m_Instances.DestroyResource(handle);
            instance.m_EventManager->RemoveListener(handle);
            instance.IsExecChanged = true;
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
        sol::table native = lua["Native"].get_or_create<sol::table>();
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

    void ScriptEngine::MarkCoroutineDone(Handle<Coroutine> handle)
    {
        auto& instance = GetInstance();
        auto task = instance.m_Coroutines.GetResource(handle);
        if (task) task->DoneFlag.store(true, std::memory_order_release);
    }

    Handle<Coroutine> ScriptEngine::StartCoroutineAPI(Handle<ScriptInstance> owner, sol::function func)
    {
        auto& instance = GetInstance();
        sol::thread runner = sol::thread::create(instance.LuaState.lua.lua_state());
        sol::coroutine co = sol::coroutine(runner.state(), func);
        auto result = co();
        lua_State* T = runner.state();
        int top = lua_gettop(T);
        AE_CORE_INFO("[Coroutine] raw stack size after resume: {0}", top);
        for (int i = 1; i <= top; i++)
        {
            int t = lua_type(T, i);
            AE_CORE_INFO("[Coroutine] stack[{0}] type={1} | tointeger={2} | tonumber={3}", 
                i, lua_typename(T, t), lua_tointeger(T, i), lua_tonumber(T, i));
        }
        if (!result.valid()) 
        {
            sol::error err = result;
            AE_CORE_ERROR("[Script] Coroutine failed to start: {0}", err.what());
            return {};
        }

        if (co.runnable()) 
        {
            CoroutineTask task;
            task.Owner = owner;
            task.Runner = std::move(runner);
            task.Co = co;
            task.Type = (WaitType)result[result.return_count() - 1].get<int>();
            if (task.Type == WaitType::Time) task.Timer = result[0].get<float>();
            else if (task.Type == WaitType::Frame) task.Frames = result[0].get<int>();
            else if (task.Type == WaitType::Event) task.AwaitEvent = result[0].get<std::string>();
            auto handle = instance.m_Coroutines.SaveResource(task);
            auto stored = instance.m_Coroutines.GetResource(handle);
            if (stored) 
            {
                stored->Self = handle;
                if (stored->Type == WaitType::Event)
                {
                    stored->EventCbHandle = instance.m_EventManager->AddNativeListener(
                        stored->AwaitEvent, 
                        [stored](const ScriptArgs&) { MarkCoroutineDone(stored->Self); }
                    );
                }
            }
            return handle;
        }

        return Handle<Coroutine>::MakeInvalid();
    }

    void ScriptEngine::KillCoroutineAPI(Handle<Coroutine> handle)
    {
        auto& instance = GetInstance();
        auto task = instance.m_Coroutines.GetResource(handle);
        if (!task) return;
        if (task->Type == WaitType::Event && task->EventCbHandle.IsValid()) 
            instance.m_EventManager->RemoveNativeListener(task->EventCbHandle, task->AwaitEvent);
        instance.m_Coroutines.DestroyResource(handle);
    }

    void ScriptEngine::UpdateCoroutines(Timestep ts)
    {
        auto& instance = GetInstance();
        instance.m_Coroutines.Loop([&](CoroutineTask& task) 
        {
            if (!instance.m_Instances.GetResource(task.Owner)) 
            {
                KillCoroutineAPI(task.Self);
                return;
            }

            bool shouldResume = false;
            switch (task.Type) 
            {
                case WaitType::Time:
                    task.Timer -= (float)ts;
                    if (task.Timer <= 0.0f) shouldResume = true;
                    break;
                case WaitType::Frame:
                    task.Frames--;
                    if (task.Frames <= 0) shouldResume = true;
                    break;
                case WaitType::Job:
                case WaitType::Event:
                    if (task.DoneFlag.load(std::memory_order_acquire)) shouldResume = true;
                    break;
                default: shouldResume = true; break;
            }

            if (shouldResume) 
            {
                if (task.Type == WaitType::Event) 
                {
                    instance.m_EventManager->RemoveNativeListener(task.EventCbHandle, task.AwaitEvent);
                    task.EventCbHandle = {};
                }

                task.DoneFlag.store(false);
                auto result = task.Co(); 
                if (!result.valid()) 
                {
                    sol::error err = result;
                    AE_CORE_ERROR("[Script] Coroutine error: {0}", err.what());
                    KillCoroutineAPI(task.Self);
                    return;
                }

                if (task.Co.runnable()) 
                {
                    task.Type = (WaitType)result[1].get<int>();
                    if (task.Type == WaitType::Time) task.Timer = result[0].get<float>();
                    else if (task.Type == WaitType::Frame) task.Frames = result[0].get<int>();
                    else if (task.Type == WaitType::Event) 
                    {
                        task.AwaitEvent = result[0].get<std::string>();
                        task.EventCbHandle = instance.m_EventManager->AddNativeListener(task.AwaitEvent, [&task](const ScriptArgs&) 
                        {
                            MarkCoroutineDone(task.Self);
                        });
                    }
                }
                else KillCoroutineAPI(task.Self);
            }
        });
    }
}