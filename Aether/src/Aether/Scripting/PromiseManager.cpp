#include "aepch.h"
#include "Aether/Scripting/PromiseManager.h"

namespace Aether {

    void PromiseManager::Init()
    {
        m_Promises.Init();
        m_Queue.reserve(32);
        m_NextQueue.reserve(32);
    }

    void PromiseManager::Shutdown()
    {
        m_Promises.Shutdown();
        m_Queue.clear();
        m_NextQueue.clear();
    }

    Handle<Promise> PromiseManager::CreatePromise()
    {
        auto handle = m_Promises.CreateResource();
        auto* p = m_Promises.GetResource(handle);
        if (p != nullptr) p->m_SelfHandle = handle;
        return handle;
    }

    Promise* PromiseManager::GetPromise(Handle<Promise> handle)
    {
        return m_Promises.GetResource(handle);
    }

    void PromiseManager::DestroyPromise(Handle<Promise> handle)
    {
        m_Promises.DestroyResource(handle);
    }

    void PromiseManager::QueueSettle(Handle<Promise> handle)
    {
        m_NextQueue.push_back(handle);
    }

    void PromiseManager::Flush()
    {
        uint32_t guard = 0;
        if (m_NextQueue.empty()) return;

        do 
        {
            std::swap(m_Queue, m_NextQueue);
            m_NextQueue.clear();

            for (size_t i = 0; i < m_Queue.size(); i++)
            {
                Handle<Promise> handle = m_Queue[i];

                Promise* p = m_Promises.GetResource(handle);
                if (p == nullptr) continue; 

                const ScriptTable result = p->m_Result;
                const bool success = p->IsFulfilled();
                auto handlers = std::move(p->m_Handlers);
                auto finallyCbs = std::move(p->m_OnFinally);
                p->m_Handlers.clear();
                p->m_OnFinally.clear();

                for (auto& handler : handlers)
                {
                    bool handlerRan = false;
                    ScriptTable out = result;
                    if (success && handler.OnFulfilled) { out = handler.OnFulfilled(result); handlerRan = true; }
                    else if (!success && handler.OnRejected) { out = handler.OnRejected(result); handlerRan = true; }

                    if (handler.NextPromise.IsValid())
                    {
                        Promise* next = m_Promises.GetResource(handler.NextPromise);
                        if (next != nullptr)
                        {
                            if (handlerRan || success) next->Resolve(out);
                            else next->Reject(out);
                        }
                    }
                }

                for (auto& cb : finallyCbs) cb();
            }
            m_Queue.clear();
        } 
        while(!m_NextQueue.empty() && ++guard < m_RecursionDepth);

        if (!m_NextQueue.empty())
            AE_CORE_WARN("[Promise] Flush hit max recursion depth {0} with {1} promises still pending for next frame.", 
                        m_RecursionDepth, m_NextQueue.size());
    }
    
    void PromiseManager::SetRecursionDepth(uint32_t depth)
    {
        m_RecursionDepth = depth;
    }
}