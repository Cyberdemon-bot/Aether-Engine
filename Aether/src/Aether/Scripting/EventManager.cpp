#include "aepch.h"
#include "Aether/Scripting/EventManager.h"
#include "Aether/Core/Log.h"

namespace Aether {

    void EventManager::Init()
    {
        m_Listeners.Init();
        m_Queue.reserve(32);
        m_NextQueue.reserve(32);
    }

    void EventManager::Shutdown()
    {
        m_Listeners.Shutdown();
        m_Queue.clear();
        m_NextQueue.clear();
    }

    void EventManager::FireEvent(const std::string& event_name, const std::vector<sol::object>& args)
    {
        m_NextQueue.push_back({event_name, args});
    }

    void EventManager::AddListener(Handle<ScriptInstance> script, const std::string& event_name, sol::main_protected_function callback)
    {
        auto [it, inserted] = m_KeyTable.try_emplace(event_name, Aether::Handle<Aether::ListenerList>::MakeInvalid());
        if (!m_Listeners.IsValid(it->second))
            it->second = m_Listeners.CreateResource();
        
        auto& listeners = *m_Listeners.GetResource(it->second);
        for (size_t i = 0; i < listeners.size(); i++)
            if (listeners[i].script.Blend() == script.Blend())
            {
                listeners[i].callback = callback;
                return;
            }
        listeners.push_back({script, callback});
    }

    void EventManager::RemoveListener(Handle<ScriptInstance> script, const std::string& event_name)
    {
        
        auto it = m_KeyTable.find(event_name);
        if (it == m_KeyTable.end()) return;

        auto* listeners = m_Listeners.GetResource(it->second);
        for (size_t i = 0; i < listeners->size(); i++)
        {
            auto& listener = (*listeners)[i];
            if (listener.script.Blend() == script.Blend())
            {
                std::swap(listener, listeners->back());
                listeners->pop_back();
                break;
            }
        }
    }

    void EventManager::RemoveListener(Handle<ScriptInstance> script)
    {
        for (auto& [name, handle] : m_KeyTable)
        {
            auto* listeners = m_Listeners.GetResource(handle);
            if (!listeners) continue;

            for (size_t i = 0; i < listeners->size(); i++)
            {
                auto& listener = (*listeners)[i];
                if (listener.script.Blend() == script.Blend())
                {
                    std::swap(listener, listeners->back());
                    listeners->pop_back();
                    break;
                }
            }
        }
    }

    void EventManager::SetRecursionDepth(uint32_t depth)
    {
        m_RecursionDepth = depth;
    }

    void EventManager::Flush()
    {
        uint32_t guard = 0;
        if (m_RecursionDepth == 0) return;
        do
        {
            std::swap(m_Queue, m_NextQueue);
            m_NextQueue.clear();    
            
            for (auto& event : m_Queue)
            {
                auto it = m_KeyTable.find(event.name);
                if (it == m_KeyTable.end()) continue;

                auto* listeners = m_Listeners.GetResource(it->second);
                if (!listeners) continue;

                for (auto& listener : *listeners)
                {
                    auto result = listener.callback(sol::as_args(event.args));
                    if (!result.valid())
                    {
                        sol::error err = result;
                        AE_CORE_ERROR("[Events] Error in listener '{0}': {1}", event.name, err.what());
                    }
                }
            }
        }
        while(!m_NextQueue.empty() && ++guard < m_RecursionDepth);

        if (!m_NextQueue.empty())
            AE_CORE_WARN("[Events] Flush hit recursion depth {0} with {1} events still pending", 
                        m_RecursionDepth, m_NextQueue.size());

        m_Queue.clear();
    }
}