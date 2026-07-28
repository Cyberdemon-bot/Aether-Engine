#include "aepch.h"
#include "Aether/Core/Assert.h"
#include "Aether/Core/JobSystem.h"

namespace Aether {

    namespace 
    {
        thread_local int t_ThreadIndex = -1;
    }

    void JobSystem::Init(uint32_t numThreads)
    {
        s_ActiveJobCount = 0;
        s_Stop = false;

        s_Queues.reserve(numThreads);
        for (uint32_t i = 0; i < numThreads; ++i)
            s_Queues.emplace_back(CreateScope<SPMCDeque<Job, 4096>>());

        s_Workers.reserve(numThreads);
        for (uint32_t i = 0; i < numThreads; ++i)
            s_Workers.emplace_back([this, i]() { WorkerThread(i); });

        AE_CORE_INFO("JobSystem initialized with {0} threads", numThreads);
    }

    void JobSystem::Shutdown()
    {
        s_Stop.store(true, std::memory_order_release);
        s_Semaphore.Release(static_cast<int>(s_Workers.size()));
        s_WaitCondition.notify_all();

        for (std::thread& worker : s_Workers)
        {
            if (worker.joinable())
                worker.join();
        }
        s_Workers.clear();
        s_Queues.clear();
        s_ActiveJobCount.store(0);

        {
            std::lock_guard<std::mutex> lock(s_InjectorMutex);
            s_Injector.clear();
        }
    }

    void JobSystem::SubmitJob(Job job)
    {
        if (s_Stop.load(std::memory_order_relaxed)) return;

        s_ActiveJobCount.fetch_add(1, std::memory_order_relaxed);

        if (t_ThreadIndex >= 0) s_Queues[t_ThreadIndex]->Push(std::move(job));
        else
        {
            std::lock_guard<std::mutex> lock(s_InjectorMutex);
            s_Injector.push_back(std::move(job));
        }

        s_Semaphore.Release();
    }

    void JobSystem::SubmitJob(Job job, Delegate<void()> callback) 
    {
        SubmitJob([this, job = std::move(job), cb = std::move(callback)]() mutable 
        {
            job();
            s_Completions.Push(std::move(cb));
        });
    }

    void JobSystem::FlushCompletions() 
    {
        Delegate<void()> cb;
        while (s_Completions.Pop(cb)) cb();
    }

    bool JobSystem::TryPopInjector(Job& out)
    {
        std::lock_guard<std::mutex> lock(s_InjectorMutex);
        if (s_Injector.empty()) return false;
        out = std::move(s_Injector.front());
        s_Injector.pop_front();
        return true;
    }

    bool JobSystem::TryStealJob(Job& out, uint32_t selfIndex)
    {
        uint32_t count = static_cast<uint32_t>(s_Queues.size());
        for (uint32_t offset = 1; offset < count; ++offset)
        {
            uint32_t victim = (selfIndex + offset) % count;
            if (s_Queues[victim]->Steal(out))
                return true;
        }
        return false;
    }

    void JobSystem::WorkerThread(uint32_t index)
    {
        t_ThreadIndex = static_cast<int>(index);

        while (!s_Stop.load(std::memory_order_relaxed))
        {
            Job job;
            bool found = s_Queues[index]->Pop(job)
                    || TryStealJob(job, index)
                    || TryPopInjector(job);

            if (found)
            {
                job();
                if (s_ActiveJobCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    std::lock_guard<std::mutex> lock(s_WaitMutex);
                    s_WaitCondition.notify_all();
                }
            }
            else s_Semaphore.Acquire();
        }
    }

    void JobSystem::WaitAll()
    {
        while (s_ActiveJobCount.load(std::memory_order_acquire) > 0)
        {
            Job job;
            bool found = TryPopInjector(job);

            if (!found)
            {
                for (auto& queue : s_Queues)
                    if (queue->Steal(job)) { found = true; break; }
            }

            if (found)
            {
                job();
                if (s_ActiveJobCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    std::lock_guard<std::mutex> lock(s_WaitMutex);
                    s_WaitCondition.notify_all();
                }
            }
            else
            {
                std::unique_lock<std::mutex> lock(s_WaitMutex);
                s_WaitCondition.wait_for(lock, std::chrono::microseconds(50),
                    [this] { return s_ActiveJobCount.load(std::memory_order_acquire) == 0; });
            }
        }
    }
}