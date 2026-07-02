#include "aepch.h"
#include "JobSystem.h"
#include "Aether/Core/Base.h"

namespace Aether {

    void JobSystem::Init(uint32_t numThreads)
    {
        s_ActiveJobCount = 0;
        s_Stop = false;
        for (uint32_t i = 0; i < numThreads; ++i)
        {
            s_Workers.emplace_back([this]() { WorkerThread(); });
        }
        AE_CORE_INFO("JobSystem initialized with {0} threads", numThreads);
    }

    void JobSystem::Shutdown()
    {
        s_Stop.store(true);
        s_Condition.notify_all();
        s_WaitCondition.notify_all();
        
        for (std::thread& worker : s_Workers)
        {
            if (worker.joinable())
                worker.join();
        }
        s_Workers.clear();

        std::queue<Job> empty;
        std::swap(s_JobQueue, empty);
        s_ActiveJobCount.store(0);
    }

    void JobSystem::SubmitJob(Job job)
    {
        if (s_Stop.load()) return;
        s_ActiveJobCount++;
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
                s_Condition.wait(lock, [this]{ return s_Stop.load() || !s_JobQueue.empty(); });
                
                if (s_Stop.load()) return;
                
                job = std::move(s_JobQueue.front());
                s_JobQueue.pop();
            }
            
            job();

            if (s_ActiveJobCount.fetch_sub(1) == 1)
            {
                std::lock_guard<std::mutex> lock(s_WaitMutex);
                s_WaitCondition.notify_all(); 
            }
        }
    }

    void JobSystem::WaitAll()
    {
        while (s_ActiveJobCount.load() > 0)
        {
            Job job;
            bool foundJob = false;

            {
                std::unique_lock<std::mutex> lock(s_QueueMutex);
                if (!s_JobQueue.empty())
                {
                    job = std::move(s_JobQueue.front());
                    s_JobQueue.pop();
                    foundJob = true;
                }
            }

            if (foundJob)
            {
                job();
                if (s_ActiveJobCount.fetch_sub(1) == 1)
                {
                    std::lock_guard<std::mutex> lock(s_WaitMutex);
                    s_WaitCondition.notify_all();
                }
            }
            else
            {
                std::unique_lock<std::mutex> lock(s_WaitMutex);
                if (s_ActiveJobCount.load() > 0) s_WaitCondition.wait(lock);
            }
        }
    }

}