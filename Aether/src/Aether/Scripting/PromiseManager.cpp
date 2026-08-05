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

    Handle<Promise> PromiseManager::CreatePromise()
    {
        auto handle = m_Promises.CreateResource();
        auto* p = m_Promises.GetResource(handle);
        if (p != nullptr) p->SelfHandle = handle;
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

    void PromiseManager::Resolve(Handle<Promise> handle, ScriptTable result)
    {
        Settle(handle, true, result);
    }

    void PromiseManager::Reject(Handle<Promise> handle, ScriptTable error)
    {
        Settle(handle, false, error);
    }

    void PromiseManager::Settle(Handle<Promise> handle, bool success, ScriptTable result)
    {
        Promise* p = m_Promises.GetResource(handle);
        if (p == nullptr || p->IsSettled()) return;

        p->CurrentState = success ? Promise::State::Fulfilled : Promise::State::Rejected;
        p->Result = result;

        QueueSettle(handle);
    }

    void PromiseManager::QueueSettle(Handle<Promise> handle)
    {
        m_NextQueue.push_back(handle);
    }

    Handle<Promise> PromiseManager::Then(Handle<Promise> handle,
                                        Delegate<ScriptTable(const ScriptTable&)> onFulfilled,
                                        Delegate<ScriptTable(const ScriptTable&)> onRejected)
    {
        Promise* p = m_Promises.GetResource(handle);
        if (p == nullptr) return Handle<Promise>::MakeInvalid();

        Handle<Promise> next = CreatePromise();
        Promise::Handler handler;
        handler.OnFulfilled = std::move(onFulfilled);
        handler.OnRejected = std::move(onRejected);
        handler.NextPromise = next;
        p->Handlers.push_back(handler);

        if (p->IsSettled()) QueueSettle(handle);

        return next;
    }

    Handle<Promise> PromiseManager::Catch(Handle<Promise> handle,
                                        Delegate<ScriptTable(const ScriptTable&)> onRejected)
    {
        return Then(handle, nullptr, std::move(onRejected));
    }

    void PromiseManager::Finally(Handle<Promise> handle, Delegate<void(const ScriptTable&)> onFinally)
    {
        if (!onFinally) return;

        Promise* p = m_Promises.GetResource(handle);
        if (p == nullptr) return;

        p->FinallyCallbacks.emplace_back(std::move(onFinally));
        if (p->IsSettled()) QueueSettle(handle);
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

                const ScriptTable result = p->Result;
                const bool success = p->IsFulfilled();
                auto handlers = std::move(p->Handlers);
                auto finallyCbs = std::move(p->FinallyCallbacks);

                for (auto& handler : handlers)
                {
                    bool handlerRan = false;
                    ScriptTable out = result;

                    if (success && handler.OnFulfilled)
                    {
                        out = handler.OnFulfilled(result);
                        handlerRan = true;
                    }
                    else if (!success && handler.OnRejected)
                    {
                        out = handler.OnRejected(result);
                        handlerRan = true;
                    }

                    if (handler.NextPromise.IsValid())
                    {
                        Promise* next = m_Promises.GetResource(handler.NextPromise);
                        if (next != nullptr)
                        {
                            if (handlerRan || success) Resolve(handler.NextPromise, out);
                            else Reject(handler.NextPromise, out);
                        }
                    }
                }

                for (auto& cb : finallyCbs) cb(result);

                if (p->Aggregate)
                {
                    for (auto& child : p->Aggregate->children)
                        Reject(child, {});
                    p->Aggregate->children.clear();
                }
                m_DestroyQueue.push_back(handle);
            }
            m_Queue.clear();
        }
        while (!m_NextQueue.empty() && ++guard < m_RecursionDepth);

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
            Resolve(aggregate, {});
            return aggregate;
        }

        uint32_t total = static_cast<uint32_t>(promises.size());

        if (total == 0)
        {
            Resolve(aggregate, {});
            return aggregate;
        }

        Promise* aggPtr = GetPromise(aggregate);
        if (aggPtr)
        {
            AggregateState state;
            state.total    = total;
            state.counter  = 0;
            state.rejected = false;
            state.settled  = false;
            state.results.resize(total);
            aggPtr->Aggregate = state;
        }

        uint32_t validIdx = 0;
        for (uint32_t i = 0; i < (uint32_t)promises.size(); i++)
        {
            Promise* p = GetPromise(promises[i]);
            if (p == nullptr)
            {
                Promise* agg = GetPromise(aggregate);
                if (agg && agg->Aggregate)
                {
                    auto& state = *agg->Aggregate;
                    state.results[i] = {}; 
                    if (++state.counter == state.total)
                        Resolve(aggregate, ScriptTable::Make(state.results));
                }
                continue;
            }

            uint32_t capturedIdx = i;

            Then(promises[i],
                [this, aggregate, capturedIdx](const ScriptTable& result) -> ScriptTable
                {
                    Promise* agg = GetPromise(aggregate);
                    if (!agg || !agg->Aggregate) return {};
                    auto& state = *agg->Aggregate;
                    if (state.rejected) return {};
                    state.results[capturedIdx] = result;
                    if (++state.counter == state.total)
                        Resolve(aggregate, ScriptTable::Make(state.results));
                    return {};
                },
                [this, aggregate](const ScriptTable& error) -> ScriptTable
                {
                    Promise* agg = GetPromise(aggregate);
                    if (!agg || !agg->Aggregate) return {};
                    auto& state = *agg->Aggregate;
                    if (state.rejected) return {};
                    state.rejected = true;
                    Reject(aggregate, error);
                    return {};
                }
            );
        }
        aggPtr->Aggregate->children = std::move(promises);
        return aggregate;
    }

    Handle<Promise> PromiseManager::Race(std::vector<Handle<Promise>> promises)
    {
        auto aggregate = CreatePromise();
        if (promises.empty()) return aggregate;

        uint32_t total = 0;
        for (auto& h : promises)
            if (GetPromise(h) != nullptr) total++;

        if (total == 0)
        {
            Resolve(aggregate, {});
            return aggregate;
        }

        Promise* aggPtr = GetPromise(aggregate);
        if (aggPtr)
        {
            AggregateState state;
            state.total   = total;
            state.settled = false;
            aggPtr->Aggregate = state;
        }

        for (auto& h : promises)
        {
            Promise* p = GetPromise(h);
            if (p == nullptr) continue;

            Then(h,
                [this, aggregate](const ScriptTable& result) -> ScriptTable
                {
                    Promise* agg = GetPromise(aggregate);
                    if (!agg || !agg->Aggregate) return {};
                    auto& state = *agg->Aggregate;
                    if (state.settled) return {};
                    state.settled = true;
                    Resolve(aggregate, result);
                    return {};
                },
                [this, aggregate](const ScriptTable& error) -> ScriptTable
                {
                    Promise* agg = GetPromise(aggregate);
                    if (!agg || !agg->Aggregate) return {};
                    auto& state = *agg->Aggregate;
                    if (state.settled) return {};
                    state.settled = true;
                    Reject(aggregate, error);
                    return {};
                }
            );
        }
        aggPtr->Aggregate->children = std::move(promises);
        return aggregate;
    }

    void PromiseManager::SetRecursionDepth(uint32_t depth)
    {
        m_RecursionDepth = depth;
    }

} 