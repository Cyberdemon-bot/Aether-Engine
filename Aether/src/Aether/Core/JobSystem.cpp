#include "JobSystem.h"
#include "Aether/Core/Base.h"

namespace Aether {

    std::vector<std::thread> JobSystem::s_Workers;
    std::queue<Job> JobSystem::s_JobQueue;
    std::mutex JobSystem::s_QueueMutex;
    std::condition_variable JobSystem::s_Condition;
    bool JobSystem::s_Stop = false;

    void JobSystem::Init(uint32_t numThreads)
    {
        s_Stop = false;
        for (uint32_t i = 0; i < numThreads; ++i)
        {
            s_Workers.emplace_back(WorkerThread);
        }
        AE_CORE_INFO("JobSystem initialized with {0} threads", numThreads);
    }

    void JobSystem::Shutdown()
    {
        {
            std::unique_lock<std::mutex> lock(s_QueueMutex);
            s_Stop = true;
        }
        s_Condition.notify_all();
        
        for (std::thread& worker : s_Workers)
        {
            if (worker.joinable())
                worker.join();
        }
        s_Workers.clear();
    }

    void JobSystem::SubmitJob(Job job)
    {
        {
            std::unique_lock<std::mutex> lock(s_QueueMutex);
            s_JobQueue.push(std::move(job));
        }
        s_Condition.notify_one();
    }

    void JobSystem::WorkerThread()
    {
        while (true)
        {
            Job job;
            {
                std::unique_lock<std::mutex> lock(s_QueueMutex);
                s_Condition.wait(lock, []{ return s_Stop || !s_JobQueue.empty(); });
                
                if (s_Stop && s_JobQueue.empty())
                    return;
                
                job = std::move(s_JobQueue.front());
                s_JobQueue.pop();
            }
            
            job();
        }
    }

}