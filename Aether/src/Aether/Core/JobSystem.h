#pragma once

#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <thread>
#include <deque> 

#include "Aether/Core/Base.h"
#include "Aether/Container/MPSCQueue.h"
#include "Aether/Container/SPMCDeque.h"
#include "Aether/Core/Semaphore.h"
#include "Aether/Core/Delegate.h"

namespace Aether {

    using Job = Delegate<void()>;

    struct WorkerQueue
    {
        Scope<SPMCDeque<Job, 4096>> queue;
        std::vector<Job> temp_buffer;
    };
    class AETHER_API JobSystem
    {
    public:
        void Init(uint32_t numThreads = SYS_THREAD_NUM);
        void Shutdown();

        void SubmitJob(Job job);
        void SubmitJob(Job job, Delegate<void()> callback);

        void FlushCompletions();
        void WaitAll();

        template<typename Func, typename Arr>
        void ParallelFor(uint32_t totalCount, uint32_t chunkSize, Arr* arr, Func&& task)
        {
            if (totalCount == 0) return;

            uint32_t numThreads = static_cast<uint32_t>(m_Workers.size());
            if (numThreads == 0) numThreads = 1;

            uint32_t maxChunks = 1024;
            uint32_t effectiveChunkSize = std::max(chunkSize, (totalCount + maxChunks - 1) / maxChunks);

            if (totalCount <= effectiveChunkSize)
            {
                for (uint32_t i = 0; i < totalCount; ++i) task(arr[i]);
                return;
            }

            uint32_t jobCount = (totalCount + effectiveChunkSize - 1) / effectiveChunkSize;

            for (uint32_t i = 0; i < jobCount; ++i)
            {
                uint32_t startIdx = i * effectiveChunkSize;
                uint32_t endIdx = std::min(startIdx + effectiveChunkSize, totalCount);

                SubmitJob(AE_MAKE_LAMBDA((task, startIdx, endIdx, arr), (), void,
                    for (uint32_t j = startIdx; j < endIdx; ++j) task(arr[j]);
                ));
            }
            WaitAll();
        }

    private:
        void WorkerThread(uint32_t index);
        bool TryStealJob(Job& out, uint32_t selfIndex);
        bool TryPopInjectorBatch(Job& outJob, uint32_t workerIndex);

        std::vector<std::thread> m_Workers;
        std::vector<WorkerQueue> m_Queues;

        MPSCQueue<Job, 4096> m_Completions;
        std::vector<Job> m_FallbackCompletions;
        std::mutex m_FallbackMutex;

        std::deque<Job> m_Injector;
        std::mutex m_InjectorMutex;

        Semaphore m_Semaphore{0};
        std::atomic<bool> m_Stop{false};

        std::atomic<uint32_t> m_ActiveJobCount{0};
        std::condition_variable m_WaitCondition;
        std::mutex m_WaitMutex;

        float m_FillRatio = 0.5;
        float m_FlushRatio = 0.25;
    };
}