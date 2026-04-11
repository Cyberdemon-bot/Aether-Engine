#pragma once
#include "Aether/Core/Base.h"
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <atomic>

namespace Aether {

    using Job = std::function<void()>;

    class AETHER_API JobSystem
    {
    public:
        static void Init(uint32_t numThreads = SYS_THREAD_NUM);
        static void Shutdown();
        
        static void SubmitJob(Job job);
        static void WaitAll();

        template<typename Func, typename Arr>
        static void ParallelFor(uint32_t totalCount, uint32_t chunkSize, Arr arr, Func&& task) 
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
        static void WorkerThread();
        
        static std::vector<std::thread> s_Workers;
        static std::queue<Job> s_JobQueue;
        static std::mutex s_QueueMutex;
        static std::condition_variable s_Condition;
        static std::atomic<bool> s_Stop;

        static std::atomic<uint32_t> s_ActiveJobCount; 
        static std::condition_variable s_WaitCondition; 
        static std::mutex s_WaitMutex;
    };

}