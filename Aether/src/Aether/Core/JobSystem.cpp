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
        m_ActiveJobCount = 0;
        m_Stop = false;

        m_Queues.reserve(numThreads);
        for (uint32_t i = 0; i < numThreads; ++i)
            m_Queues.emplace_back(CreateScope<SPMCDeque<Job, 4096>>());

        m_Workers.reserve(numThreads);
        for (uint32_t i = 0; i < numThreads; ++i)
            m_Workers.emplace_back([this, i]() { WorkerThread(i); });

        AE_CORE_INFO("JobSystem initialized with {0} threads", numThreads);
    }

    void JobSystem::Shutdown()
    {
        m_Stop.store(true, std::memory_order_release);
        m_Semaphore.Release(static_cast<int>(m_Workers.size()));
        m_WaitCondition.notify_all();

        for (std::thread& worker : m_Workers)
        {
            if (worker.joinable())
                worker.join();
        }
        m_Workers.clear();
        m_Queues.clear();
        m_ActiveJobCount.store(0);

        {
            std::lock_guard<std::mutex> lock(m_InjectorMutex);
            m_Injector.clear();
        }
    }

    void JobSystem::SubmitJob(Job job)
    {
        if (m_Stop.load(std::memory_order_relaxed)) return;

        m_ActiveJobCount.fetch_add(1, std::memory_order_relaxed);

        if (t_ThreadIndex >= 0) m_Queues[t_ThreadIndex]->Push(std::move(job));
        else
        {
            std::lock_guard<std::mutex> lock(m_InjectorMutex);
            m_Injector.push_back(std::move(job));
        }

        m_Semaphore.Release();
    }

    void JobSystem::SubmitJob(Job job, Delegate<void()> callback) 
    {
        SubmitJob([this, job = std::move(job), cb = std::move(callback)]() mutable 
        {
            job();
            m_Completions.Push(std::move(cb));
        });
    }

    void JobSystem::FlushCompletions() 
    {
        Delegate<void()> cb;
        while (m_Completions.Pop(cb)) cb();
    }

    bool JobSystem::TryPopInjector(Job& out)
    {
        std::lock_guard<std::mutex> lock(m_InjectorMutex);
        if (m_Injector.empty()) return false;
        out = std::move(m_Injector.front());
        m_Injector.pop_front();
        return true;
    }

    bool JobSystem::TryStealJob(Job& out, uint32_t selfIndex)
    {
        uint32_t count = static_cast<uint32_t>(m_Queues.size());
        for (uint32_t offset = 1; offset < count; ++offset)
        {
            uint32_t victim = (selfIndex + offset) % count;
            if (m_Queues[victim]->Steal(out))
                return true;
        }
        return false;
    }

    void JobSystem::WorkerThread(uint32_t index)
    {
        t_ThreadIndex = static_cast<int>(index);

        while (!m_Stop.load(std::memory_order_relaxed))
        {
            Job job;
            bool found = m_Queues[index]->Pop(job)
                    || TryStealJob(job, index)
                    || TryPopInjector(job);

            if (found)
            {
                job();
                if (m_ActiveJobCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    std::lock_guard<std::mutex> lock(m_WaitMutex);
                    m_WaitCondition.notify_all();
                }
            }
            else m_Semaphore.Acquire();
        }
    }

    void JobSystem::WaitAll()
    {
        while (m_ActiveJobCount.load(std::memory_order_acquire) > 0)
        {
            Job job;
            bool found = TryPopInjector(job);

            if (!found)
            {
                for (auto& queue : m_Queues)
                    if (queue->Steal(job)) { found = true; break; }
            }

            if (found)
            {
                job();
                if (m_ActiveJobCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
                {
                    std::lock_guard<std::mutex> lock(m_WaitMutex);
                    m_WaitCondition.notify_all();
                }
            }
            else
            {
                std::unique_lock<std::mutex> lock(m_WaitMutex);
                m_WaitCondition.wait_for(lock, std::chrono::microseconds(50),
                    [this] { return m_ActiveJobCount.load(std::memory_order_acquire) == 0; });
            }
        }
    }
}