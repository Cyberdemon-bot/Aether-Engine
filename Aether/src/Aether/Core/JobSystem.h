#pragma once
#include <functional>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>

namespace Aether {

    using Job = std::function<void()>;

    class JobSystem
    {
    public:
        static void Init(uint32_t numThreads = std::thread::hardware_concurrency());
        static void Shutdown();
        
        static void SubmitJob(Job job);
        
    private:
        static void WorkerThread();
        
        static std::vector<std::thread> s_Workers;
        static std::queue<Job> s_JobQueue;
        static std::mutex s_QueueMutex;
        static std::condition_variable s_Condition;
        static bool s_Stop;
    };

}