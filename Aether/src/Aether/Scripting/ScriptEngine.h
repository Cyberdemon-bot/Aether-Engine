#pragma once
#include "Aether/Scene/Scene.h"
#include "Aether/Events/Event.h"
#include <sol/sol.hpp>
#include <type_traits>

namespace Aether 
{

    template <typename T, typename = void>
    struct has_get_attributes : std::false_type {};

    template <typename T>
    struct has_get_attributes<T, std::void_t<decltype(T::get_attributes())>> : std::true_type {};

    // Dò tìm hàm get_props()
    template <typename T, typename = void>
    struct has_get_props : std::false_type {};

    template <typename T>
    struct has_get_props<T, std::void_t<decltype(T::get_props())>> : std::true_type {};

    // Dò tìm hàm get_ops()
    template <typename T, typename = void>
    struct has_get_ops : std::false_type {};

    template <typename T>
    struct has_get_ops<T, std::void_t<decltype(T::get_ops())>> : std::true_type {};
    struct InstanceHandle
    {
        int index = -1, generation = -1;
        bool IsValid() const { return index >= 0 && generation >= 0; }
    };

    struct InstanceSlot
    {
        sol::environment env;
        int generation = 0;
    };
    class ScriptEngine
    {
    public:
        static void Init();
        static void Shutdown();
        static void RegisterTypes();

        static InstanceHandle CreateInstance(Scene* scene, Entity entity);
        static void DestroyInstance(InstanceHandle handle);
        static void UpdateInstance(InstanceHandle handle, Timestep ts);
        static void LoadScript(InstanceHandle handle, const std::string& path);
    private:
        static ScriptEngine& GetInstance();

        sol::state s_LuaState;
        static sol::meta_function OpNameToMeta(std::string_view name);
        std::vector<InstanceSlot> m_Instances;
        std::vector<uint32_t> FreeList;


        template <typename UType>
        static void FlushOpGroup(UType& utype, sol::meta_function mf, const std::vector<sol::object>& fns)
        {
            switch (fns.size())
            {
            case 1: utype[mf] = fns[0]; break;
            case 2: utype[mf] = sol::overload(fns[0].as<sol::function>(),fns[1].as<sol::function>()); break;
            case 3: utype[mf] = sol::overload(fns[0].as<sol::function>(),fns[1].as<sol::function>(),fns[2].as<sol::function>()); break;
            case 4: utype[mf] = sol::overload(fns[0].as<sol::function>(),fns[1].as<sol::function>(),
                        fns[2].as<sol::function>(),fns[3].as<sol::function>()); break;
            default:
                AE_CORE_WARN("[ScriptEngine] Too many overloads for one op, only first 4 bound");
                break;
            }
        }

        template <typename T>
        static void BindReflectedType()
        {
            auto& instance = GetInstance();
            auto utype = instance.s_LuaState.new_usertype<T>(T::get_name(), sol::no_constructor);

            if constexpr (has_get_attributes<T>::value)
            {
                auto attb = T::get_attributes();
                std::apply([&](auto&&... att) {
                    ([&](auto&& a) {
                        auto [name, member, readonly] = a;
                        if (readonly) utype[name] = sol::readonly(member);
                        else          utype[name] = member;
                    }(att), ...); 
                }, attb);
            }

            if constexpr (has_get_props<T>::value)
            {
                auto props = T::get_props();
                std::apply([&](auto&&... prop) {
                    ([&](auto&& p) {
                        auto [name, getter, setter] = p;
                        
                        using SetterType = std::decay_t<decltype(setter)>;
                        
                        if constexpr (std::is_same_v<SetterType, std::nullptr_t>) {
                            utype[name] = sol::property(getter);
                        }
                        else {
                            utype[name] = sol::property(getter, setter);
                        }
                    }(prop), ...);
                }, props);
            }

            if constexpr (has_get_ops<T>::value)
            {
                auto ops = T::get_ops();
                std::optional<sol::meta_function> currentMF;
                std::vector<sol::object> currentFns;

                auto flush = [&]() {
                    if (currentMF && !currentFns.empty())
                        FlushOpGroup(utype, *currentMF, currentFns);
                    currentFns.clear();
                    currentMF.reset();
                };

                std::apply([&](auto&&... op) {
                    ([&](auto&& o) {
                        auto [name, fn] = o;
                        sol::meta_function mf = ScriptEngine::OpNameToMeta(name);
                        if (currentMF && *currentMF != mf)
                            flush();
                        currentMF = mf;
                        currentFns.push_back(sol::make_object(instance.s_LuaState, fn));
                    }(op), ...);
                }, ops);
                flush(); 
            }
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
                    AE_CORE_ERROR("[Script Error in {0}] {1}", name, err.what());
                }
            }
        }
    };
}