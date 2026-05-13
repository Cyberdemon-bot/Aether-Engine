#pragma once

#include "Aether/Container/Handle.h"
#include "Aether/Container/ResourcePool.h"
#include "Aether/Core/Base.h"
#include <sol/sol.hpp>
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <vector>

namespace Aether {
    struct ScriptInstance;
    struct ScriptCallback;

    struct ScriptEvent 
    {
        std::string name;
        std::vector<sol::object> args;
    };

    struct EventListener 
    {
        Handle<ScriptInstance> script;
        sol::protected_function callback;
    };

    struct ScriptValue
    {
        enum class Type { Nil, Bool, Int, Float, String, Vec3 };
        Type type = Type::Nil;
        union { bool b; int i; float f; };
        std::string str;
        glm::vec3 vec;

        template<typename T> T As() const;
        static ScriptValue FromSolObject(const sol::object& obj);
    };

    class ScriptArgs
    {
    public:
        template<typename T>
        T Get(uint32_t index) const
        {
            if (index >= args.size()) 
            {
                AE_CORE_WARN("[ScriptArgs] Index {0} out of range", index);
                return T{};
            }
            return args[index].As<T>();
        }
    private:
        std::vector<ScriptValue> args;
        friend class ScriptEventManager;
    };

    struct NativeListener
    {
        Delegate<void(const ScriptArgs& args)> callback;
    };

    class ScriptEventManager
    {
    public:
        ScriptEventManager(sol::state& lua);
        ~ScriptEventManager() = default;

        void FireEvent(const std::string& event_name, const std::vector<sol::object>& args);

        void AddListener(Handle<ScriptInstance> script, const std::string& event_name, sol::protected_function callback);
        Handle<ScriptCallback> AddNativeListener(const std::string& event_name, Delegate<void(const ScriptArgs& args)> callback);
        void RemoveListener(Handle<ScriptInstance> script, const std::string& event_name);
        void RemoveNativeListener(Handle<ScriptCallback> handle, const std::string& event_name);

        void Flush();
    private:
        std::unordered_map<std::string, std::vector<EventListener>> m_Listeners;
        std::unordered_map<std::string, ResourcePool<Handle<ScriptCallback>, NativeListener>> m_NativeListeners;
        std::vector<ScriptEvent> m_Queue;
        sol::state& m_lua;
    };

    template<> bool ScriptValue::As() const;
    template<> int ScriptValue::As() const;
    template<> float ScriptValue::As() const;
    template<> std::string ScriptValue::As() const;
    template<> glm::vec3 ScriptValue::As() const;
}