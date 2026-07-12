#include "aepch.h"
#include "Aether/Core/Assert.h"
#include "Aether/Core/JobSystem.h"

namespace Aether {

    namespace 
    {
        thread_local int t_ThreadIndex = -1;
    }

    void JobQueue::Push(Job job)
    {
        int64_t b = m_Bottom.load(std::memory_order_relaxed);
        int64_t t = m_Top.load(std::memory_order_acquire);
        AE_CORE_ASSERT(b - t < static_cast<int64_t>(CAPACITY), "JobQueue overflow");
        m_Buffer[b % CAPACITY] = std::move(job);
        std::atomic_thread_fence(std::memory_order_release);
        m_Bottom.store(b + 1, std::memory_order_relaxed);
    }

    bool JobQueue::Pop(Job& out)
    {
        int64_t b = m_Bottom.load(std::memory_order_relaxed) - 1;
        m_Bottom.store(b, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        int64_t t = m_Top.load(std::memory_order_relaxed);

        if (t > b)
        {
            m_Bottom.store(b + 1, std::memory_order_relaxed);
            return false;
        }

        out = m_Buffer[b % CAPACITY];
        if (t == b)
        {
            if (!m_Top.compare_exchange_strong(t, t + 1))
            {
                m_Bottom.store(b + 1, std::memory_order_relaxed);
                return false;
            }
            m_Bottom.store(b + 1, std::memory_order_relaxed);
        }

        return true;
    }

    bool JobQueue::Steal(Job& out)
    {
        int64_t t = m_Top.load(std::memory_order_acquire);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        int64_t b = m_Bottom.load(std::memory_order_acquire);

        if (t >= b) return false;
        out = m_Buffer[t % CAPACITY];
        return m_Top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst);
    }

    void JobSystem::Init(uint32_t numThreads)
    {
        s_ActiveJobCount = 0;
        s_Stop = false;

        s_Queues.reserve(numThreads);
        for (uint32_t i = 0; i < numThreads; ++i)
            s_Queues.emplace_back(std::make_unique<JobQueue>());

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