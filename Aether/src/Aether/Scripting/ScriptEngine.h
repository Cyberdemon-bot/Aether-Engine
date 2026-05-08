#pragma once

#include <sol/sol.hpp>
#include <type_traits>
#include <magic_enum/magic_enum.hpp>
#include <unordered_map>
#include <string>

#include "Aether/Core/Base.h"
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

    struct ScriptTag;
    struct EnvTag;
    struct BytecodeTag;
    enum class CollisionType;
    class Scene;

    struct InstanceSlot
    {
        int generation = 0;
        Handle<EnvTag> env_hanle = Handle<EnvTag>::MakeInvalid();
        bool has_error = false;
        bool is_active = true;
        int exec_order = 0;
        Scene* ctx = nullptr;

        std::unordered_map<std::string, sol::protected_function> exposed_funcs;
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

    struct CollisionData
    {
        uint32_t entity;
        CollisionType type;

        glm::vec3 contactPoint;
        glm::vec3 contactNormal;
    };

    class AETHER_API ScriptEngine
    {
    public:
        static void Init();
        static void Shutdown();
        static void RegisterTypes();

        static Handle<ScriptTag> CreateInstance(Scene* scene, Entity entity, Handle<BytecodeTag> bh);
        static void DestroyInstance(Handle<ScriptTag> handle);
        static void StartInstance(Handle<ScriptTag> handle);
        static void FireEvent(const std::string& event_name);

        static Handle<BytecodeTag> LoadScript(const std::string& path);

        static void SetActiveStage(Handle<ScriptTag> handle, bool active);
        static bool GetActiveStage(Handle<ScriptTag> handle);

    private:
        static ScriptEngine& GetInstance();

        LuaWorker LuaState;
        std::optional<ScriptEventManager> m_EventManager;
        static sol::meta_function OpNameToMeta(std::string_view name);
        ResourcePool<Handle<ScriptTag>, InstanceSlot> m_Instances;
        ResourcePool<Handle<BytecodeTag>, sol::bytecode> m_Sources;
        std::vector<std::pair<Entity, Handle<ScriptTag>>> m_DestroyQueue;
        bool IsExecChanged = false;

        static void FlushEvent();
        static void UpdateInstance(Handle<ScriptTag> handle, Timestep ts);
        static void OnInstanceCollision(Handle<ScriptTag> handle, CollisionData data);
        static bool IsExecOrderChanged();
        static int GetExecOrder(Handle<ScriptTag> handle);

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
        static void CallMethod(InstanceSlot& slot, const std::string& name, Args&&... args) 
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

        static sol::object CallInstanceAPI(Handle<ScriptTag> handle, const std::string& name, const std::vector<sol::object>& args)
        {
            auto& instance = GetInstance();
            auto slot = instance.m_Instances.GetResource(handle);
            if (slot == nullptr) return sol::lua_nil;

            auto it = slot->exposed_funcs.find(name);
            if (it == slot->exposed_funcs.end()) return sol::lua_nil;

            sol::protected_function_result result = it->second(sol::as_args(args));
            if (!result.valid())
            {
                sol::error err = result;
                AE_CORE_ERROR("[Script] CallInstanceAPI error in '{0}': {1}", name, err.what());
                return sol::lua_nil;
            }
            return result;
        }

        static void MarkExecOrderChanged() {GetInstance().IsExecChanged = true;}
        static void PushDestroyQueue(Entity ent, Handle<ScriptTag> handle) { GetInstance().m_DestroyQueue.push_back({ent, handle}); }

        friend struct ScriptSelfBinding;
        friend struct SceneBinding;
        friend class Scene;
    };
} 