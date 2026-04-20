#pragma once
#include "Aether/Core/Base.h"
#include "Aether/Scene/Scene.h"
#include "Aether/Container/ResourcePool.h"
#include <sol/sol.hpp>
#include <type_traits>
#include <magic_enum/magic_enum.hpp>
namespace Aether {

    template <typename T, typename = void> struct HasGetProps : std::false_type {};
    template <typename T> struct HasGetProps<T, std::void_t<decltype(T::get_props())>> : std::true_type {};

    template <typename T, typename = void> struct HasGetOps : std::false_type {};
    template <typename T> struct HasGetOps<T, std::void_t<decltype(T::get_ops())>> : std::true_type {};

    template <typename T, typename = void> struct HasGetMethods : std::false_type {};
    template <typename T> struct HasGetMethods<T, std::void_t<decltype(T::get_methods())>> : std::true_type {};

    template <typename T, typename = void> struct HasGetFuncs : std::false_type {};
    template <typename T> struct HasGetFuncs<T, std::void_t<decltype(T::get_funcs())>> : std::true_type {};

    struct ScriptTag;
    struct EnvTag;

    struct InstanceSlot
    {
        int generation = 0;
        Handle<EnvTag> env_hanle = Handle<EnvTag>::MakeInvalid();
        bool has_error = false;
        bool is_active = true;
    };

    struct LuaWorker 
    {
        sol::state lua;
        size_t env_count = 0;
        
        Handle<EnvTag> CreateEnvironment() 
        {
            Handle<EnvTag> handle = env_pool.CreateResource(lua, sol::create, lua.globals());
            env_count++;
            return handle;
        }
        
        void RemoveEnvironment(Handle<EnvTag> handle) 
        {
            if (env_count > 0) env_count--;
            auto env = env_pool.GetResource(handle);
            if (env == nullptr) return;
            (*env) = sol::lua_nil;
            env_pool.DestroyResource(handle);
        }

        ResourcePool<Handle<EnvTag>, sol::environment> env_pool;
    };

    class AETHER_API ScriptEngine
    {
    public:
        static void Init();
        static void Shutdown();
        static void Update();
        static void RegisterTypes();

        static Handle<ScriptTag> CreateInstance(Scene* scene, Entity entity);
        static void DestroyInstance(Handle<ScriptTag> handle);
        static void StartInstance(Handle<ScriptTag> handle);
        static void UpdateInstance(Handle<ScriptTag> handle, Timestep ts);
        static void PushEventToInstance(Handle<ScriptTag> handle, Event event);
        static void LoadScript(Handle<ScriptTag> handle, const std::string& path);
        static void SetActiveStage(Handle<ScriptTag> handle, bool active);
        static bool GetActiveStage(Handle<ScriptTag> handle);

    private:
        static ScriptEngine& GetInstance();

        LuaWorker LuaState;
        static sol::meta_function OpNameToMeta(std::string_view name);
        ResourcePool<Handle<ScriptTag>, InstanceSlot> m_Instances;

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
            auto entries = magic_enum::enum_entries<T>();
            for (const auto& [value, name] : entries)
                dataTable[name] = static_cast<typename std::underlying_type<T>::type>(value);

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
        static void CallMethod(InstanceSlot slot, const std::string& name, Args&&... args) 
        {
            auto& instance = GetInstance();
            auto env = instance.LuaState.env_pool.GetResource(slot.env_hanle);
            if (env == nullptr) return;
            sol::protected_function func = (*env)[name];
            
            if (func.valid()) 
            {
                auto result = func(std::forward<Args>(args)...); 
                if (!result.valid()) 
                {
                    sol::error err = result;
                    slot.has_error = true;
                    AE_CORE_ERROR("[Script Error in {0}] {1}", name, err.what());
                }
            }
        }
    };
} 