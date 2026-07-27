#pragma once

#include "Aether/Core/Delegate.h"
#include "Aether/Container/Handle.h"
#include "Aether/Container/ResourcePool.h"
#include "Aether/Scripting/ScriptTable.h"
#include <sol/sol.hpp>
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>
#include <vector>
#include <tuple>

namespace Aether {
    struct ScriptInstance;
    struct ListenerList;

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

    class EventManager
    {
    public:
        EventManager() = default;

        void Init();
        void Shutdown();

        void FireEvent(const std::string& event_name, const std::vector<sol::object>& args);

        void AddListener(Handle<ScriptInstance> script, const std::string& event_name, sol::main_protected_function callback);
        void RemoveListener(Handle<ScriptInstance> script);
        void RemoveListener(Handle<ScriptInstance> script, const std::string& event_name);

        void SetRecursionDepth(uint32_t depth);
        void Flush();
    private:
        EventManager(const EventManager&) = delete;
        EventManager& operator=(const EventManager&) = delete;
        EventManager(EventManager&&) = default;
        EventManager& operator=(EventManager&&) = default;

        std::unordered_map<std::string, Handle<ListenerList>> m_KeyTable;
        ResourcePool<Handle<ListenerList>, std::vector<EventListener>> m_Listeners;
        std::vector<ScriptEvent> m_Queue;
        std::vector<ScriptEvent> m_NextQueue;

        uint32_t m_RecursionDepth = 3;
    };
}