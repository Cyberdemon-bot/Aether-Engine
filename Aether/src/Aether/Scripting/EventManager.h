#pragma once

#include <string>
#include <vector>
#include <string_view>
#include <sol/sol.hpp>
#include <glm/glm.hpp>
#include "Aether/Core/Delegate.h"
#include "Aether/Container/Handle.h"
#include "Aether/Container/ResourcePool.h"
#include "Aether/Container/Table.h"
#include "Aether/Scripting/ScriptTable.h"

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
        bool is_native = false;
        sol::main_protected_function callback;
        Delegate<bool(const ScriptTable&)> native_callback;
    };

    struct ListenerList
    {
        std::vector<Handle<EventListener>> list;
    };

    class EventManager
    {
    public:
        EventManager() = default;

        void Init();
        void Shutdown();

        void FireEvent(std::string_view event_name, const std::vector<sol::object>& args);

        Handle<EventListener> CreateListener(std::string_view event_name, sol::main_protected_function callback);
        Handle<EventListener> CreateListener(std::string_view event_name, const Delegate<bool(const ScriptTable&)>& native_callback);
        void DestroyListener(Handle<EventListener> handle);
        void RemoveEvent(std::string_view event_name);

        void SetRecursionDepth(uint32_t depth);
        void Flush();
    private:
        EventManager(const EventManager&) = delete;
        EventManager& operator=(const EventManager&) = delete;
        EventManager(EventManager&&) = default;
        EventManager& operator=(EventManager&&) = default;
        
        Table<Handle<ListenerList>, ListenerList> m_Keys;
        ResourcePool<Handle<EventListener>, EventListener> m_Listeners;
        
        std::vector<ScriptEvent> m_Queue;
        std::vector<ScriptEvent> m_NextQueue;
        std::vector<Handle<EventListener>> m_DestroyQueue;
        std::vector<ScriptTable> m_ArgsBuffer;

        uint32_t m_RecursionDepth = 3;
    };
}