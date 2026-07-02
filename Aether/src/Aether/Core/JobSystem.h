#pragma once
#include "Aether/Core/Base.h"
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>

namespace Aether {

    using Job = Delegate<void()>;

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
        void WorkerThread();
        
        std::vector<std::thread> s_Workers;
        std::queue<Job> s_JobQueue;
        std::mutex s_QueueMutex;
        std::condition_variable s_Condition;
        std::atomic<bool> s_Stop;

        std::atomic<uint32_t> s_ActiveJobCount; 
        std::condition_variable s_WaitCondition; 
        std::mutex s_WaitMutex;
    };

}