#pragma once

#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <thread>
#include <deque> 

#include "Aether/Core/Base.h"
#include "Aether/Container/MSPCQueue.h"
#include "Aether/Container/SPMCDeque.h"
#include "Aether/Core/Semaphore.h"
#include "Aether/Core/Delegate.h"

namespace Aether {

    using Job = Delegate<void()>;

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
        void ParallelFor(uint32_t totalCount, uint32_t chunkSize, Arr arr, Func&& task)
        {
            if (totalCount == 0) return;

            if (totalCount <= chunkSize)
            {
                for (uint32_t i = 0; i < totalCount; ++i) task(arr[i]);
                return;
            }

            uint32_t jobCount = (totalCount + chunkSize - 1) / chunkSize;

            for (uint32_t i = 0; i < jobCount; ++i)
            {
                uint32_t startIdx = i * chunkSize;
                uint32_t endIdx = std::min(startIdx + chunkSize, totalCount);

                SubmitJob(AE_MAKE_LAMBDA((&task, startIdx, endIdx, arr), (), void,
                    for (uint32_t j = startIdx; j < endIdx; ++j) task(arr[j]);
                ));
            }
            WaitAll();
        }

    private:
        void WorkerThread(uint32_t index);
        bool TryStealJob(Job& out, uint32_t selfIndex);
        bool TryPopInjector(Job& out);

        std::vector<std::thread> s_Workers;
        std::vector<Scope<SPMCDeque<Job, 4096>>> s_Queues;
        MSPCQueue<Delegate<void()>, 1024> s_Completions;

        std::deque<Job> s_Injector;
        std::mutex s_InjectorMutex;

        Semaphore s_Semaphore{0};
        std::atomic<bool> s_Stop{false};

        std::atomic<uint32_t> s_ActiveJobCount{0};
        std::condition_variable s_WaitCondition;
        std::mutex s_WaitMutex;
    };
}