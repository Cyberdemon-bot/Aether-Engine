#pragma once

#include "Aether/Container/Handle.h"
#include "Aether/Container/ResourcePool.h"
#include "Aether/Core/Base.h"
#include "Aether/Scripting/ScriptValue.h"
#include <sol/sol.hpp>
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <tuple>

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
        sol::main_protected_function callback;
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

        ScriptEventManager(const ScriptEventManager&) = delete;
        ScriptEventManager& operator=(const ScriptEventManager&) = delete;

        ScriptEventManager(ScriptEventManager&&) = default;
        ScriptEventManager& operator=(ScriptEventManager&&) = default;

        void FireEvent(const std::string& event_name, const std::vector<sol::object>& args);

        void AddListener(Handle<ScriptInstance> script, const std::string& event_name, sol::main_protected_function callback);
        Handle<ScriptCallback> AddNativeListener(const std::string& event_name, Delegate<void(const ScriptArgs& args)> callback);
        void RemoveListener(Handle<ScriptInstance> script);
        void RemoveListener(Handle<ScriptInstance> script, const std::string& event_name);
        void RemoveNativeListener(Handle<ScriptCallback> handle);
        void RemoveNativeListener(Handle<ScriptCallback> handle, const std::string& event_name);

        void Flush();
    private:
        std::unordered_map<std::string, std::vector<EventListener>> m_Listeners;
        std::unordered_map<std::string, ResourcePool<Handle<ScriptCallback>, NativeListener>> m_NativeListeners;
        std::vector<ScriptEvent> m_Queue;
        sol::state& m_lua;
    };
}