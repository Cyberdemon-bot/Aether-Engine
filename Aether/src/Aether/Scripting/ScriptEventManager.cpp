#include "aepch.h"
#include "Aether/Scripting/ScriptEventManager.h"
#include "Aether/Core/Log.h"

namespace Aether {
    template<> bool ScriptValue::As() const { return b; }
    template<> int ScriptValue::As() const { return i; }
    template<> float ScriptValue::As() const { return f; }
    template<> std::string ScriptValue::As() const { return str; }
    template<> glm::vec3 ScriptValue::As() const { return vec; }


    ScriptEventManager::ScriptEventManager(sol::state& lua)
        : m_lua(lua)
    {
        m_Listeners.reserve(32);
        m_NativeListeners.reserve(32);
        m_Queue.reserve(32);
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
            converted.args.reserve(event.args.size());
            for (const auto& arg : event.args)
                converted.args.push_back(ScriptValue::FromSolObject(arg));
            it->second.Loop([converted](const NativeListener& listener){
                listener.callback(converted);
            });
        }

        m_Queue.clear();
    }

    ScriptValue ScriptValue::FromSolObject(const sol::object& obj)
    {
        ScriptValue v;
        if (!obj.valid()) { v.type = Type::Nil; return v; }
        if (obj.is<bool>()) { v.type = Type::Bool; v.b = obj.as<bool>(); return v; }
        if (obj.is<int>()) { v.type = Type::Int; v.i = obj.as<int>(); return v; }
        if (obj.is<float>()) { v.type = Type::Float; v.f = obj.as<float>(); return v; }
        if (obj.is<std::string>()) { v.type = Type::String; v.str = obj.as<std::string>(); return v; }
        if (obj.is<glm::vec3>()) { v.type = Type::Vec3; v.vec = obj.as<glm::vec3>(); return v; }
        AE_CORE_WARN("[ScriptValue] Unknown sol::object type, defaulting to Nil");
        return v;
    }
}