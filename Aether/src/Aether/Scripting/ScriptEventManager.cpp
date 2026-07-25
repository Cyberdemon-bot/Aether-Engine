#include "aepch.h"
#include "Aether/Scripting/ScriptEventManager.h"
#include "Aether/Core/Log.h"

namespace Aether {

    void ScriptEventManager::Init()
    {
        m_Listeners.reserve(32);
        m_NativeListeners.reserve(32);
        m_Queue.reserve(32);
    }

    void ScriptEventManager::Shutdown()
    {
        m_Listeners.clear();
        m_NativeListeners.clear();
        m_Queue.clear();
    }

    void ScriptEventManager::FireEvent(const std::string& event_name, const std::vector<sol::object>& args)
    {
        m_Queue.push_back({event_name, args});
    }

    void ScriptEventManager::AddListener(Handle<ScriptInstance> script, const std::string& event_name, sol::main_protected_function callback)
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

    Handle<ScriptCallback> ScriptEventManager::AddNativeListener(const std::string& event_name, Delegate<void(const ScriptArgs& args)> callback)
    {
        auto& listeners = m_NativeListeners[event_name];
        return listeners.SaveResource({callback});
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

    void ScriptEventManager::RemoveNativeListener(Handle<ScriptCallback> handle, const std::string& event_name)
    {
        auto& listeners = m_NativeListeners[event_name];
        listeners.DestroyResource(handle);
    }

    void ScriptEventManager::RemoveListener(Handle<ScriptInstance> script)
    {
        for (auto& [name, listeners] : m_Listeners)
        {
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

        for (auto& event : m_Queue)
        {
            auto it = m_NativeListeners.find(event.name);
            if (it == m_NativeListeners.end()) continue;
            ScriptArgs converted;
            for (const auto& arg : event.args)
                converted.Pushback(FromSolObject(arg));
            it->second.Loop([converted](const NativeListener& listener)
            {
                listener.callback(converted);
            });
        }

        m_Queue.clear();
    }

}