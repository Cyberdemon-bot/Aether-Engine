#pragma once

#include <sol/sol.hpp>
#include <sol/coroutine.hpp>
#include <type_traits>
#include <magic_enum/magic_enum.hpp>
#include <unordered_map>
#include <string>

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include "Aether/Scene/Entity.h"
#include "Aether/Core/Timestep.h"
#include "Aether/Container/ResourcePool.h"
#include "Aether/Scripting/ScriptEventManager.h"
namespace Aether {

    template <typename T, typename = void> struct HasGetProps : std::false_type {};
    template <typename T> struct HasGetProps<T, std::void_t<decltype(T::get_props())>> : std::true_type {};

    template <typename T, typename = void> struct HasGetOps : std::false_type {};
    template <typename T> struct HasGetOps<T, std::void_t<decltype(T::get_ops())>> : std::true_type {};

    template <typename T, typename = void> struct HasGetMethods : std::false_type {};
    template <typename T> struct HasGetMethods<T, std::void_t<decltype(T::get_methods())>> : std::true_type {};

    template <typename T, typename = void> struct HasGetFuncs : std::false_type {};
    template <typename T> struct HasGetFuncs<T, std::void_t<decltype(T::get_funcs())>> : std::true_type {};

    struct ScriptInstance;
    struct Environment;
    struct Bytecode;
    struct Coroutine;
    enum class CollisionType;
    class Scene;

    enum class WaitType
    {
        None, Time, Frame, Event, Job 
    };

    struct CoroutineTask
    {
        Handle<ScriptInstance> Owner;
        Handle<Coroutine> Self;
        sol::thread Runner;
        sol::coroutine Co;
        WaitType Type = WaitType::None;
        float Timer = 0.0f;
        int Frames = 0;
        std::atomic<bool> DoneFlag{false}; 
        Handle<ScriptCallback> EventCbHandle; 
        std::string AwaitEvent;
        ScriptValue JobResult;

        CoroutineTask() = default;
        CoroutineTask(const CoroutineTask&) = delete;
        CoroutineTask& operator=(const CoroutineTask&) = delete;

        CoroutineTask(CoroutineTask&& other) noexcept
            : Owner(other.Owner),
            Self(other.Self),
            Runner(std::move(other.Runner)),
            Co(std::move(other.Co)),
            Type(other.Type),
            Timer(other.Timer),
            Frames(other.Frames),
            EventCbHandle(other.EventCbHandle),
            AwaitEvent(std::move(other.AwaitEvent)),
            JobResult(std::move(other.JobResult))
        {
            DoneFlag.store(other.DoneFlag.load(std::memory_order_relaxed),
                        std::memory_order_relaxed);
            other.Owner = {};
            other.Self = {};
            other.Type = WaitType::None;
        }

        CoroutineTask& operator=(CoroutineTask&& other) noexcept
        {
            if (this != &other)
            {
                Owner = other.Owner;
                Self = other.Self;
                Runner = std::move(other.Runner);
                Co = std::move(other.Co);
                JobResult = std::move(other.JobResult);
                Type = other.Type;
                Timer = other.Timer;
                Frames = other.Frames;
                EventCbHandle = other.EventCbHandle;
                AwaitEvent = std::move(other.AwaitEvent);
                DoneFlag.store(other.DoneFlag.load(std::memory_order_relaxed),
                            std::memory_order_relaxed);
                other.Owner = {};
                other.Self = {};
                other.Type = WaitType::None;
            }
            return *this;
        }
    };

    struct Exposed
    {
        std::string name;
        sol::protected_function func;
    };
    struct InstanceSlot
    {
        int generation = 0;
        Handle<Environment> env_handle = Handle<Environment>::MakeInvalid();
        Handle<Bytecode> code_handle = Handle<Bytecode>::MakeInvalid();
        bool has_error = false;
        bool is_active = true;
        int exec_order = 0;
        Scene* ctx = nullptr;
        std::vector<Exposed> exposed_funcs;
    };

    struct LuaWorker 
    {
        sol::state lua;
        size_t env_count = 0;
        
        Handle<Environment> CreateEnvironment() 
        {
            Handle<Environment> handle = env_pool.CreateResource(lua, sol::create, lua.globals());
            env_count++;
            return handle;
        }
        
        void RemoveEnvironment(Handle<Environment> handle) 
        {
            if (env_count > 0) env_count--;
            auto env = env_pool.GetResource(handle);
            if (env == nullptr) return;
            (*env) = sol::lua_nil;
            env_pool.DestroyResource(handle);
        }

        ResourcePool<Handle<Environment>, sol::environment> env_pool;
    };

    struct CollisionData
    {
        UUID entityID;
        CollisionType type;

        glm::vec3 contactPoint;
        glm::vec3 contactNormal;
    };

    struct ScriptSource
    {
        sol::bytecode bytecode;
    };

    struct NativeFunc
    {
        std::string name;
        Delegate<ScriptValue(const ScriptArgs&)> native;
    };


    class AETHER_API ScriptEngine
    {
    public:
        static void Init();
        static void Shutdown();

        static Handle<ScriptInstance> CreateInstance(Scene* scene, Entity entity, Handle<Bytecode> bh);
        static void DestroyInstance(Handle<ScriptInstance> handle);
        static void StartInstance(Handle<ScriptInstance> handle);

        template<typename... Args>
        static void FireEvent(const std::string& event_name, Args&&... args)
        {
            auto& instance = GetInstance();
            auto& lua = instance.LuaState.lua;
            std::vector<sol::object> sol_args = {
                sol::make_object(lua, std::forward<Args>(args))...
            };
            instance.m_EventManager->FireEvent(event_name, sol_args);
        }

        static Handle<ScriptCallback> AddListener(const std::string& event_name, Delegate<void(const ScriptArgs& args)> callback);
        static void RemoveListener(Handle<ScriptCallback> handle, const std::string& event_name);
        static void ImportNativeFunc(const std::string& name, Delegate<ScriptValue(const ScriptArgs&)> func);
        static Handle<Bytecode> LoadScript(const std::string& path);
        static Handle<Bytecode> LoadScriptSource(const std::string& source);

        static void SetActiveStage(Handle<ScriptInstance> handle, bool active);
        static bool GetActiveStage(Handle<ScriptInstance> handle);

        static Handle<Coroutine> StartCoroutineAPI(Handle<ScriptInstance> owner, sol::function func);
        static void KillCoroutineAPI(Handle<Coroutine> handle);
        static void UpdateCoroutines(Timestep ts);
        static void MarkCoroutineDone(Handle<Coroutine> handle);
    private:
        static ScriptEngine& GetInstance();

        LuaWorker LuaState;
        std::optional<ScriptEventManager> m_EventManager;
        static sol::meta_function OpNameToMeta(std::string_view name);
        ResourcePool<Handle<ScriptInstance>, InstanceSlot> m_Instances;
        ResourcePool<Handle<Bytecode>, ScriptSource> m_Sources;
        ResourcePool<Handle<Coroutine>, CoroutineTask> m_Coroutines;
        std::vector<Handle<ScriptInstance>> m_DestroyQueue;
        std::vector<NativeFunc> m_NativeFuncs;
        bool IsExecChanged = false;

        static void FlushEvent();
        static void UpdateInstance(Handle<ScriptInstance> handle, Timestep ts);
        static void OnInstanceCollision(Handle<ScriptInstance> handle, CollisionData data);
        static bool IsExecOrderChanged();
        static int GetExecOrder(Handle<ScriptInstance> handle);
        static void RegisterBinding();

        template<typename Binder>
        static void BindType(const std::string& Namespace = "")
        {
            auto& instance = GetInstance();
            auto& lua = instance.LuaState.lua;
            sol::table table = lua.globals();
            if (!Namespace.empty()) table = lua[Namespace].get_or_create<sol::table>();
            using TargetType = typename Binder::Type;
            auto utype = table.new_usertype<TargetType>(Binder::get_name(), sol::call_constructor, sol::constructors<TargetType()>());

            if constexpr (HasGetProps<Binder>::value)
            {
                ForEachTuple(Binder::get_props(), [&utype](auto&& item)
                {
                    auto name = std::get<0>(item);
                    auto lambdas = std::get<1>(item);
                    std::apply([&](auto&&... args) 
                    {
                        utype.set(name, sol::property(std::forward<decltype(args)>(args)...));
                    }, lambdas);
                });
            }

            if constexpr (HasGetMethods<Binder>::value)
            {
                ForEachTuple(Binder::get_methods(), [&utype](auto&& item)
                {
                    auto name = std::get<0>(item);
                    auto lambdas = std::get<1>(item);
                    auto overloaded_funcs = std::apply([](auto&&... fns) 
                    {
                        return sol::overload(std::forward<decltype(fns)>(fns)...);
                    }, lambdas);
                    utype.set_function(name, overloaded_funcs);
                });
            }

            if constexpr (HasGetOps<Binder>::value) 
            {
                ForEachTuple(Binder::get_ops(), [&utype](auto&& item) 
                {
                    std::string name = std::get<0>(item);
                    auto lambdas = std::get<1>(item);

                    auto overloaded_ops = std::apply([](auto&&... fns) {
                        return sol::overload(std::forward<decltype(fns)>(fns)...);
                    }, lambdas);

                    utype.set_function(OpNameToMeta(name), overloaded_ops);
                });
            }
        }

        template<typename Binder>
        static void BindModule(const std::string& Namespace = "")
        {
            auto& instance = GetInstance();
            auto& lua = instance.LuaState.lua;
            sol::table table = lua.globals();
            if (!Namespace.empty()) table = lua[Namespace].get_or_create<sol::table>();
            if constexpr (HasGetFuncs<Binder>::value) 
            {
                ForEachTuple(Binder::get_funcs(), [&table](auto&& item) 
                {
                    auto name = std::get<0>(item);
                    auto lambdas = std::get<1>(item);
                    auto overloaded_funcs = std::apply([](auto&&... fns) 
                    {
                        return sol::overload(std::forward<decltype(fns)>(fns)...);
                    }, lambdas);
                    table.set_function(name, overloaded_funcs);
                });
            }
        }

        template<typename T>
        static void BindEnum(const std::string& Name, const std::string& Namespace = "")
        {
            auto& instance = GetInstance();
            auto& lua = instance.LuaState.lua;
            sol::table dataTable = lua.create_table();
            sol::table reverseTable = lua.create_table(); 
            auto entries = magic_enum::enum_entries<T>();
            for (const auto& [value, name] : entries)
            {
                auto intVal = static_cast<typename std::underlying_type<T>::type>(value);
                dataTable[name] = intVal;
                reverseTable[intVal] = name;  
            }
            dataTable["Name"] = reverseTable; 

            sol::table proxyTable = lua.create_table();
            sol::table mt = lua.create_table();

            mt["__index"] = dataTable;
            mt["__newindex"] = [](sol::table t, sol::object key, sol::object value) {};
            mt[sol::meta_function::metatable] = false;

            proxyTable[sol::metatable_key] = mt;

            sol::table targetTable = lua.globals();
            if (!Namespace.empty()) targetTable = lua[Namespace].get_or_create<sol::table>();
            targetTable[Name] = proxyTable;
        }

        template<typename... Args>
        static sol::object CallSafeInstanceAPI(Handle<ScriptInstance> handle, const std::string& name, Args&&... args)
        {
            auto& instance = GetInstance();
            auto slot = instance.m_Instances.GetResource(handle);
            if (slot == nullptr || slot->has_error) return sol::lua_nil;
            auto it = std::find_if(slot->exposed_funcs.begin(), slot->exposed_funcs.end(), [name](const Exposed& data) { return data.name == name; });
            if (it == slot->exposed_funcs.end()) return sol::lua_nil;

            sol::protected_function_result result = it->func(std::forward<Args>(args)...);
            if (!result.valid())
            {
                sol::error err = result;
                slot->has_error = true;
                AE_CORE_ERROR("[Script] CallSafeInstanceAPI error in '{0}': {1}", name, err.what());
                return sol::lua_nil;
            }
            return result;
        }

        template<typename... Args>
        static sol::object CallDirectInstanceAPI(Handle<ScriptInstance> handle, const std::string& name, Args&&... args)
        {
            auto& instance = GetInstance();
            auto slot = instance.m_Instances.GetResource(handle);
            if (slot == nullptr || slot->has_error) return sol::lua_nil;
            auto env = instance.LuaState.env_pool.GetResource(slot->env_handle);
            if (env == nullptr) return sol::lua_nil;

            sol::protected_function func = (*env)[name]; if (!func.valid()) return sol::lua_nil;
            sol::protected_function_result result = func(std::forward<Args>(args)...);
            if (!result.valid())
            {
                sol::error err = result;
                AE_CORE_ERROR("[Script] CallInstanceAPI error in '{0}': {1}", name, err.what());
                return sol::lua_nil;
            }
            return result;
        }

        friend struct ScriptSelfBinding;
        friend struct AsyncBinding;
        friend class Scene;
    };
} 