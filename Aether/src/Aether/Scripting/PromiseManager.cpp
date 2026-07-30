#include "aepch.h"
#include "Aether/Scripting/PromiseManager.h"

namespace Aether {

    void PromiseManager::Init()
    {
        m_Promises.Init();
        m_Queue.reserve(32);
        m_NextQueue.reserve(32);
        m_DestroyQueue.reserve(32);
    }

    void PromiseManager::Shutdown()
    {
        m_Promises.Shutdown();
        m_Queue.clear();
        m_NextQueue.clear();
        m_DestroyQueue.clear();
    }

    Handle<Promise> PromiseManager::CreatePromise(PromiseOwnership ownership)
    {
        auto handle = m_Promises.CreateResource();
        auto* p = m_Promises.GetResource(handle);
        if (p != nullptr)
        {
            p->m_SelfHandle = handle;
            p->m_Ownership = ownership;
        }
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

                if (p->m_Ownership == PromiseOwnership::AutoReap) m_DestroyQueue.push_back(handle);
            }
            m_Queue.clear();
        } 
        while(!m_NextQueue.empty() && ++guard < m_RecursionDepth);

        for (auto& handle : m_DestroyQueue) m_Promises.DestroyResource(handle);
        m_DestroyQueue.clear();

        if (!m_NextQueue.empty())
            AE_CORE_WARN("[Promise] Flush hit max recursion depth {0} with {1} promises still pending for next frame.", 
                        m_RecursionDepth, m_NextQueue.size());
    }

    Handle<Promise> PromiseManager::All(std::vector<Handle<Promise>> promises)
    {
        auto aggregate = CreatePromise();
        
        if (promises.empty())
        {
            GetPromise(aggregate)->Resolve({});
            return aggregate;
        }

        uint32_t total = 0;
        for (auto& handle : promises)
            if (GetPromise(handle) != nullptr) total++;

        if (total == 0)
        {
            GetPromise(aggregate)->Resolve({});
            return aggregate;
        }

        Promise* aggPtr = GetPromise(aggregate);
        if (aggPtr)
        {
            AggregateState state;
            state.total = total;
            state.counter = 0;
            state.rejected = false;
            state.settled = false;
            state.results.resize(total);
            aggPtr->m_AggregateState = state;
        }

        uint32_t validIdx = 0;
        for (uint32_t i = 0; i < (uint32_t)promises.size(); i++)
        {
            Promise* p = GetPromise(promises[i]);
            if (p == nullptr) continue;

            uint32_t capturedIdx = validIdx++;

            p->Then(
                [this, aggregate, capturedIdx](const ScriptTable& result) -> ScriptTable 
                {
                    Promise* agg = GetPromise(aggregate);
                    if (!agg || !agg->m_AggregateState) return {};
                    auto& state = *agg->m_AggregateState;
                    if (state.rejected) return {};
                    state.results[capturedIdx] = result;
                    if (++state.counter == state.total)
                        agg->Resolve(ScriptTable::Make(state.results));
                    return {};
                },
                [this, aggregate](const ScriptTable& error) -> ScriptTable 
                {
                    Promise* agg = GetPromise(aggregate);
                    if (!agg || !agg->m_AggregateState) return {};
                    auto& state = *agg->m_AggregateState;
                    if (state.rejected) return {};
                    state.rejected = true;
                    agg->Reject(error);
                    return {};
                }
            );
        }
        return aggregate;
    }

    Handle<Promise> PromiseManager::Race(std::vector<Handle<Promise>> promises)
    {
        auto aggregate = CreatePromise();
        if (promises.empty()) return aggregate;

        uint32_t total = 0;
        for (auto& handle : promises)
            if (GetPromise(handle) != nullptr) total++;

        if (total == 0)
        {
            GetPromise(aggregate)->Resolve({});
            return aggregate;
        }
        
        Promise* aggPtr = GetPromise(aggregate);
        if (aggPtr)
        {
            AggregateState state;
            state.total = total;
            state.settled = false;
            aggPtr->m_AggregateState = state;
        }

        for (auto& handle : promises)
        {
            Promise* p = GetPromise(handle);
            if (p == nullptr) continue;

            p->Then(
                [this, aggregate](const ScriptTable& result) -> ScriptTable 
                {
                    Promise* agg = GetPromise(aggregate);
                    if (!agg || !agg->m_AggregateState) return {};
                    auto& state = *agg->m_AggregateState;
                    if (state.settled) return {};
                    state.settled = true;
                    agg->Resolve(result);
                    return {};
                },
                [this, aggregate](const ScriptTable& error) -> ScriptTable 
                {
                    Promise* agg = GetPromise(aggregate);
                    if (!agg || !agg->m_AggregateState) return {};
                    auto& state = *agg->m_AggregateState;
                    if (state.settled) return {};
                    state.settled = true;
                    agg->Reject(error);
                    return {};
                }
            );
        }
        return aggregate;
    }
    
    void PromiseManager::SetRecursionDepth(uint32_t depth)
    {
        m_RecursionDepth = depth;
    }
}