#include "aepch.h"
#include "Aether/Scripting/ScriptEventManager.h"
#include "Aether/Core/Log.h"

namespace Aether {
    ScriptEventManager::ScriptEventManager(sol::state& lua)
        : m_lua(lua)
    {
        m_Listeners.reserve(64);
        m_Queue.reserve(64);
    }  

    void ScriptEventManager::FireEvent(const std::string& event_name, const std::vector<sol::object>& args)
    {
        m_Queue.push_back({event_name, args});
    }

    void ScriptEventManager::AddListener(Handle<ScriptInstance> script, const std::string& event_name, sol::protected_function callback)
    {
        auto& listeners = m_Listeners[event_name];
        for (size_t i = 0; i < listeners.size(); i++)
        {
            if (listeners[i].script.Blend() == script.Blend())
            {
                listeners[i].callback = callback;
                return;
            }
        }
        listeners.push_back({script, callback});
    }

    void ScriptEventManager::RemoveListener(Handle<ScriptInstance> script, const std::string& event_name)
    {
        auto& listeners = m_Listeners[event_name];
        for (size_t i = 0; i < listeners.size(); i++)
        {
            auto& listener = listeners[i];
            if (listener.script.Blend() == script.Blend())
            {
                std::swap(listener, listeners.back());
                listeners.pop_back();
                break;
            }
        }
    }

    void ScriptEventManager::Flush()
    {
        for (auto& event : m_Queue)
        {
            auto it = m_Listeners.find(event.name);
            if (it == m_Listeners.end()) continue;

            for (auto& listener : it->second)
            {
                auto result = listener.callback(sol::as_args(event.args));
                if (!result.valid())
                {
                    sol::error err = result;
                    AE_CORE_ERROR("[Events] Error in listener '{0}': {1}", event.name, err.what());
                }
            }
        }

        m_Queue.clear();
    }
}