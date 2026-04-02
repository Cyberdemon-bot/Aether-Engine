#pragma once
#include "Aether/Scene/Scene.h"
#include "Aether/Core/Base.h"
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

    struct InstanceHandle
    {
        int index = -1, generation = -1;
        bool IsValid() const { return index >= 0 && generation >= 0; }
    };

    struct InstanceSlot
    {
        sol::environment env;
        int generation = 0;
        bool has_error = false;
    };

    class AETHER_API ScriptEngine
    {
    public:
        static void Init();
        static void Shutdown();
        static void RegisterTypes();

        static InstanceHandle CreateInstance(Scene* scene, Entity entity);
        static void DestroyInstance(InstanceHandle handle);
        static void StartInstance(InstanceHandle handle);
        static void UpdateInstance(InstanceHandle handle, Timestep ts);
        static void PushEventToInstance(InstanceHandle handle, Event event);
        static void LoadScript(InstanceHandle handle, const std::string& path);

    private:
        static ScriptEngine& GetInstance();

        sol::state s_LuaState;
        static sol::meta_function OpNameToMeta(std::string_view name);
        std::vector<InstanceSlot> m_Instances;
        std::vector<uint32_t> FreeList;

        template<typename Binder>
        static void BindType(const std::string& NS = "")
        {
            auto& instance = GetInstance();
            auto& lua = instance.s_LuaState;
            sol::table table = lua.globals();
            if (!NS.empty()) table = lua[NS].get_or_create<sol::table>();
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
        static void BindModule(const std::string& NS = "")
        {
            auto& instance = GetInstance();
            auto& lua = instance.s_LuaState;
            sol::table table = lua.globals();
            if (!NS.empty()) table = lua[NS].get_or_create<sol::table>();
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
        static void BindEnum(const std::string& Name, const std::string& NS = "")
        {
            auto& instance = GetInstance();
            auto& lua = instance.s_LuaState;

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
            if (!NS.empty()) targetTable = lua[NS].get_or_create<sol::table>();
            targetTable[Name] = proxyTable;
        }

        template<typename... Args>
        static void CallMethod(InstanceHandle handle, const std::string& name, Args&&... args) 
        {
            auto& slot = GetInstance().m_Instances[handle.index];
            sol::protected_function func = slot.env[name];
            
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