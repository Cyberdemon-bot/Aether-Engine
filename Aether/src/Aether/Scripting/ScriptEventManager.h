#pragma once

#include "Aether/Container/Handle.h"
#include <sol/sol.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <tuple>

namespace Aether {
    struct ScriptInstance;

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


    class ScriptEventManager
    {
    public:
        ScriptEventManager(sol::state& lua);
        ~ScriptEventManager() = default;

        void FireEvent(const std::string& event_name, const std::vector<sol::object>& args);

        void AddListener(Handle<ScriptInstance> script, const std::string& event_name, sol::protected_function callback);
        void RemoveListener(Handle<ScriptInstance> script, const std::string& event_name);

        void Flush();
    private:
        std::unordered_map<std::string, std::vector<EventListener>> m_Listeners;
        std::vector<ScriptEvent> m_Queue;
        sol::state& m_lua;
    };
}