#pragma once

#include <vector>
#include "Aether/Container/Handle.h"
#include "Aether/Container/ResourcePool.h"
#include "Aether/Scripting/Promise.h"

namespace Aether {

    class PromiseManager
    {
    public:
        PromiseManager() = default;

        void Init();
        void Shutdown();

        Handle<Promise> CreatePromise();
        Promise* GetPromise(Handle<Promise> handle);
        void  DestroyPromise(Handle<Promise> handle);

        void Resolve(Handle<Promise> handle, ScriptTable result);
        void Reject(Handle<Promise> handle, ScriptTable error);
        void Settle(Handle<Promise> handle, bool success, ScriptTable result);
        Handle<Promise> Then(Handle<Promise> handle,
                             const Delegate<ScriptTable(const ScriptTable&)>& onFulfilled,
                             const Delegate<ScriptTable(const ScriptTable&)>& onRejected = {});
        Handle<Promise> Catch(Handle<Promise> handle,
                              const Delegate<ScriptTable(const ScriptTable&)>& onRejected);
        void Finally(Handle<Promise> handle,
                                const Delegate<void()>& onFinally);

        Handle<Promise> All(std::vector<Handle<Promise>> promises);
        Handle<Promise> Race(std::vector<Handle<Promise>> promises);

        void Flush();
        void QueueSettle(Handle<Promise> handle);
        void SetRecursionDepth(uint32_t depth);

    private:
        PromiseManager(const PromiseManager&) = delete;
        PromiseManager& operator=(const PromiseManager&) = delete;
        PromiseManager(PromiseManager&&) = default;
        PromiseManager& operator=(PromiseManager&&) = default;

        ResourcePool<Handle<Promise>, Promise> m_Promises;
        std::vector<Handle<Promise>> m_Queue;
        std::vector<Handle<Promise>> m_NextQueue;
        std::vector<Handle<Promise>> m_DestroyQueue;

        uint32_t m_RecursionDepth = 3;
    };

} // namespace Aether