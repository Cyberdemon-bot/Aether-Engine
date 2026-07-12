#pragma once
#include "Aether/Core/Base.h"
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>
#include <array>
#include <thread>
#include <deque> 

namespace Aether {

    using Job = Delegate<void()>;

    class JobQueue
    {
    public:
        void Push(Job job);
        bool Pop(Job& out);
        bool Steal(Job& out);
    private:
        static constexpr size_t CAPACITY = 4096;
        std::array<Job, CAPACITY> m_Buffer;
        alignas(64) std::atomic<int64_t> m_Top{0};
        alignas(64) std::atomic<int64_t> m_Bottom{0};
    };

    class AETHER_API JobSystem
    {
    public:
        void Init(uint32_t numThreads = SYS_THREAD_NUM);
        void Shutdown();

        void SubmitJob(Job job);
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
        std::vector<std::unique_ptr<JobQueue>> s_Queues;

        std::deque<Job> s_Injector;
        std::mutex s_InjectorMutex;

        Semaphore s_Semaphore{0};
        std::atomic<bool> s_Stop{false};

        std::atomic<uint32_t> s_ActiveJobCount{0};
        std::condition_variable s_WaitCondition;
        std::mutex s_WaitMutex;
    };
}