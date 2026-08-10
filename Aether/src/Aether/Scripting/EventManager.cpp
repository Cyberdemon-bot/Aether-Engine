#include "aepch.h"
#include "Aether/Scripting/EventManager.h"
#include "Aether/Core/Log.h"

namespace Aether {

    void EventManager::Init()
    {
        m_Keys.Init();
        m_Listeners.Init();
        m_Queue.reserve(32);
        m_ArgsBuffer.reserve(32);
        m_NextQueue.reserve(32);
        m_DestroyQueue.reserve(32);
        m_OwnershipMap.reserve(32);
    }

    void EventManager::Shutdown()
    {
        m_Queue.clear();
        m_NextQueue.clear();
        m_ArgsBuffer.clear();
        m_DestroyQueue.clear();
        m_OwnershipMap.clear();
        m_Keys.Shutdown();
        m_Listeners.Shutdown();
    }

    void EventManager::FireEvent(std::string_view event_name, const std::vector<sol::object>& args)
    {
        m_NextQueue.push_back({std::string(event_name), args});
    }

    Handle<EventListener> EventManager::CreateListener(std::string_view event_name, sol::main_protected_function callback, Handle<ScriptInstance> owner)
    {
        Handle<ListenerList> list = m_Keys.GetOrCreate(event_name);
        auto* listIt = m_Keys.GetData(list);
        if (!listIt) return Handle<EventListener>::MakeInvalid();

        Handle<EventListener> listener = m_Listeners.CreateResource();
        auto* listenerIt = m_Listeners.GetResource(listener);
        if (!listenerIt) return Handle<EventListener>::MakeInvalid();

        listenerIt->is_native = false;
        listenerIt->callback = std::move(callback);
        listIt->list.push_back(listener);

        if (owner.index >= m_OwnershipMap.size()) m_OwnershipMap.resize(owner.index + 1);
        m_OwnershipMap[owner.index].list.push_back(listener);
        return listener;
    }

    Handle<EventListener> EventManager::CreateListener(std::string_view event_name, const Delegate<bool(const ScriptTable&)>& native_callback)
    {
        Handle<ListenerList> list = m_Keys.GetOrCreate(event_name);
        auto* listIt = m_Keys.GetData(list);
        if (!listIt) return Handle<EventListener>::MakeInvalid();

        Handle<EventListener> listener = m_Listeners.CreateResource();
        auto* listenerIt = m_Listeners.GetResource(listener);
        if (!listenerIt) return Handle<EventListener>::MakeInvalid();

        listenerIt->is_native = true;
        listenerIt->native_callback = native_callback;
        listIt->list.push_back(listener);
        return listener;
    }

    void EventManager::DestroyListener(Handle<EventListener> handle)
    {
        auto* listener = m_Listeners.GetResource(handle);
        if (!listener) return;

        if (!listener->is_native)
        {
            auto owner = listener->script_owner;
            if (owner.index >= m_OwnershipMap.size()) return;
            auto& list = m_OwnershipMap[owner.index].list;
            uint64_t target = handle.Blend();
            auto it = std::find_if(list.begin(), list.end(), [target](const auto& item) 
            {
                return item.Blend() == target;
            });

            if (it != list.end())
            {
                *it = list.back(); 
                list.pop_back(); 
            }
        }
        m_DestroyQueue.push_back(handle);
    }

    void EventManager::RemoveEvent(std::string_view event_name)
    {
        Handle<ListenerList> list = m_Keys.Search(event_name);
        if (!list.IsValid()) return;

        auto* listIt = m_Keys.GetData(list);
        if (!listIt) return;

        for (auto& handle : listIt->list) DestroyListener(handle);
        listIt->list.clear();
    }

    void EventManager::RemoveScript(Handle<ScriptInstance> owner)
    {
        if (owner.index >= m_OwnershipMap.size()) return;
        for (auto& handle : m_OwnershipMap[owner.index].list) DestroyListener(handle);
        m_OwnershipMap[owner.index].list.clear();
    }

    void EventManager::SetRecursionDepth(uint32_t depth)
    {
        m_RecursionDepth = depth;
    }

    void EventManager::Flush()
    {
        uint32_t guard = 0;
        m_Keys.Resolve();
        if (m_RecursionDepth == 0) return;
        do
        {
            std::swap(m_Queue, m_NextQueue);
            m_NextQueue.clear();

            for (auto& event : m_Queue)
            {
                Handle<ListenerList> list = m_Keys.Search(event.name);
                if (!list.IsValid()) continue;

                auto* listIt = m_Keys.GetData(list);
                if (!listIt) continue;

                for (auto handle : listIt->list)
                {
                    auto* listener = m_Listeners.GetResource(handle);
                    if (!listener) continue;

                    if (!listener->is_native)
                    {
                        auto result = listener->callback(sol::as_args(event.args));
                        if (!result.valid())
                        {
                            sol::error err = result;
                            AE_CORE_ERROR("[Events] Error in listener '{0}': {1}", event.name, err.what());
                        }
                    }
                    else
                    {
                        for (auto& arg : event.args)
                            m_ArgsBuffer.push_back(ScriptTable::FromSolObject(arg));

                        bool isContinue = listener->native_callback(ScriptTable::Make(m_ArgsBuffer));
                        m_ArgsBuffer.clear();
                        if (!isContinue) DestroyListener(handle);
                    }
                }
            }
        }
        while (!m_NextQueue.empty() && ++guard < m_RecursionDepth);

        if (!m_NextQueue.empty())
            AE_CORE_WARN("[Events] Flush hit recursion depth {0} with {1} events still pending",
                        m_RecursionDepth, m_NextQueue.size());

        for (auto& handle : m_DestroyQueue)
            m_Listeners.DestroyResource(handle);

        m_DestroyQueue.clear();
        m_Keys.ForEach([this](ListenerList& listIt)
        {
            std::erase_if(listIt.list, [this](auto handle)
            {
                return !m_Listeners.GetResource(handle);
            });
        });
    }
}