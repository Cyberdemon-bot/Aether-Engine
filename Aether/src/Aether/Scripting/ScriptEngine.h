#pragma once

#include <sol/sol.hpp>
#include <sol/coroutine.hpp>
#include <type_traits>
#include <magic_enum/magic_enum.hpp>
#include <string>

#include "Aether/Core/Delegate.h"
#include "Aether/Core/UUID.h"
#include "Aether/Scene/Entity.h"
#include "Aether/Core/Timestep.h"
#include "Aether/Container/ResourcePool.h"
#include "Aether/Scripting/PromiseManager.h"
#include "Aether/Scripting/EventManager.h"
#include "Aether/Scripting/CoroutineManager.h"
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
    enum class CollisionType;
    class Scene;

    struct Exposed
    {
        std::string name;
        sol::main_protected_function func;
    };
    struct InstanceSlot
    {
        int generation = 0;
        Handle<Environment> env_handle = Handle<Environment>::MakeInvalid();
        Handle<Bytecode> code_handle = Handle<Bytecode>::MakeInvalid();
        bool has_error = false;
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
        Delegate<ScriptTable(const ScriptTable&)> native;
    };


    class AETHER_API ScriptEngine
    {
    public:
        void Init();
        void Shutdown();

        Handle<ScriptInstance> CreateInstance(Scene* scene, Entity entity, Handle<Bytecode> bh);
        void DestroyInstance(Handle<ScriptInstance> handle);
        void StartInstance(Handle<ScriptInstance> handle);

        void Update(Timestep ts);

        template<typename... Args>
        void FireEvent(const std::string& event_name, Args&&... args)
        {
            auto& lua = LuaState.lua;
            std::vector<sol::object> sol_args = {
                sol::make_object(lua, std::forward<Args>(args))...
            };
            m_EventManager.FireEvent(event_name, sol_args);
        }

        template<typename... Args>
        ScriptTable CallMethod(Handle<ScriptInstance> handle, const std::string func_name, Args&&... args)
        {
            auto& obj = CallDirectInstanceAPI(handle, func_name, std::forward<Args>(args)...);
            return ScriptTable::FromSolObject(obj);
        }

        void ImportNativeFunc(const std::string& name, Delegate<ScriptTable(const ScriptTable&)> func);
        Handle<Bytecode> LoadScript(const std::string& source);
    private:
        LuaWorker LuaState;
        EventManager m_EventManager;
        CoroutineManager m_CoroutineManager;
        sol::meta_function OpNameToMeta(std::string_view name);
        ResourcePool<Handle<ScriptInstance>, InstanceSlot> m_Instances;
        ResourcePool<Handle<Bytecode>, ScriptSource> m_Sources;
        PromiseManager m_PromiseManager;
        std::vector<Handle<ScriptInstance>> m_DestroyQueue;
        std::vector<NativeFunc> m_NativeFuncs;
        bool IsExecChanged = false;

        void FlushEvent();
        void UpdateInstance(Handle<ScriptInstance> handle, Timestep ts);
        void OnInstanceCollision(Handle<ScriptInstance> handle, CollisionData data);
        bool IsExecOrderChanged();
        int GetExecOrder(Handle<ScriptInstance> handle);
        void RegisterBinding();

        template<typename Binder>
        void BindType(const std::string& Namespace = "")
        {
            auto& lua = LuaState.lua;
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
                ForEachTuple(Binder::get_ops(), [&utype, this](auto&& item) 
                {
                    std::string name = std::get<0>(item);
                    auto lambdas = std::get<1>(item);

                    auto overloaded_ops = std::apply([](auto&&... fns) {
                        return sol::overload(std::forward<decltype(fns)>(fns)...);
                    }, lambdas);

                    utype.set_function(this->OpNameToMeta(name), overloaded_ops);
                });
            }
        }

        template<typename Binder>
        void BindModule(const std::string& Namespace = "")
        {
            auto& lua = LuaState.lua;
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
        void BindEnum(const std::string& Name, const std::string& Namespace = "")
        {
            auto& lua = LuaState.lua;
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
        sol::object CallSafeInstanceAPI(Handle<ScriptInstance> handle, const std::string& name, Args&&... args)
        {
            auto slot = m_Instances.GetResource(handle);
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
        sol::object CallDirectInstanceAPI(Handle<ScriptInstance> handle, const std::string& name, Args&&... args)
        {
            auto slot = m_Instances.GetResource(handle);
            if (slot == nullptr || slot->has_error) return sol::lua_nil;
            auto env = LuaState.env_pool.GetResource(slot->env_handle);
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
        friend struct SceneBinding;
        friend class Scene;
    };
}