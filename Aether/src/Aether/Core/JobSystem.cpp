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
            m_Queues.emplace_back(WorkerQueue{ CreateScope<SPMCDeque<Job, 4096>>(), {} });

        m_Workers.reserve(numThreads);
        for (uint32_t i = 0; i < numThreads; ++i)
            m_Workers.emplace_back([this, i]() { WorkerThread(i); });

        AE_CORE_INFO("JobSystem initialized with {0} threads", numThreads);
    }

    void JobSystem::Shutdown()
    {
        m_Stop.store(true, std::memory_order_release);
        m_Semaphore.Release(static_cast<int>(m_Workers.size()));
        {
            std::lock_guard<std::mutex> lock(m_WaitMutex);
            m_WaitCondition.notify_all();
        }

        for (std::thread& worker : m_Workers)
            if (worker.joinable()) worker.join();
        
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

        if (t_ThreadIndex >= 0)
        {
            uint32_t workerIndex = static_cast<uint32_t>(t_ThreadIndex);
            auto& queue = m_Queues[workerIndex].queue;
            auto& temp = m_Queues[workerIndex].temp_buffer; 

            if (queue->Push(std::move(job)))
            {
                m_Semaphore.Release();
                return;
            }

            size_t flush_count = std::max<size_t>(1, static_cast<size_t>(queue->Size() * m_FlushRatio));
            temp.clear(); temp.reserve(flush_count); Job poppedJob;
            while (temp.size() < flush_count && queue->Pop(poppedJob)) temp.push_back(std::move(poppedJob));
            size_t cnt = temp.size();

            if (cnt > 0)
            {
                std::lock_guard<std::mutex> lock(m_InjectorMutex);
                m_Injector.insert(
                    m_Injector.end(),
                    std::make_move_iterator(temp.begin()),
                    std::make_move_iterator(temp.end())
                );
            }
            if (!m_Queues[workerIndex].queue->Push(std::move(job)))
            {
                std::lock_guard<std::mutex> lock(m_InjectorMutex);
                m_Injector.push_back(std::move(job));
            }
            m_Semaphore.Release();
            return;
        }

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
            if (m_Completions.Push(cb)) return;
            std::lock_guard<std::mutex> lock(m_FallbackMutex);
            m_FallbackCompletions.push_back(std::move(cb));
        });
    }

    void JobSystem::FlushCompletions() 
    {
        Delegate<void()> cb;
        while (m_Completions.Pop(cb)) cb();
        
        std::vector<Job> processList;
        {
            std::lock_guard<std::mutex> lock(m_FallbackMutex);
            if (!m_FallbackCompletions.empty()) processList.swap(m_FallbackCompletions); 
        }
        for (auto& fallbackCb : processList) fallbackCb();
    }

    bool JobSystem::TryPopInjectorBatch(Job& outJob, uint32_t workerIndex)
    {
        std::lock_guard<std::mutex> lock(m_InjectorMutex);
        if (m_Injector.empty()) return false;

        outJob = std::move(m_Injector.front());
        m_Injector.pop_front();

        if (t_ThreadIndex == static_cast<int>(workerIndex))
        {
            size_t max_fill = static_cast<size_t>(m_Queues[workerIndex].queue->Size() * m_FillRatio);
            size_t pushedCount = 0;

            while (!m_Injector.empty() && pushedCount < max_fill)
            {
                if (m_Queues[workerIndex].queue->Push(std::move(m_Injector.front())))
                {
                    m_Injector.pop_front();
                    pushedCount++;
                }
                else break;
            }
        }

        return true;
    }

    bool JobSystem::TryStealJob(Job& out, uint32_t selfIndex)
    {
        uint32_t count = static_cast<uint32_t>(m_Queues.size());
        for (uint32_t offset = 1; offset < count; ++offset)
        {
            uint32_t victim = (selfIndex + offset) % count;
            if (m_Queues[victim].queue->Steal(out))
                return true;
        }
        return false;
    }

    void JobSystem::WorkerThread(uint32_t index)
    {
        t_ThreadIndex = static_cast<int>(index);

        while (true)
        {
            Job job;
            bool found = m_Queues[index].queue->Pop(job)
                    || TryStealJob(job, index)
                    || TryPopInjectorBatch(job, index);

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
                if (m_Stop.load(std::memory_order_relaxed)) break;
                m_Semaphore.Acquire();

                if (m_Stop.load(std::memory_order_relaxed))
                {
                    bool hasMoreJobs = m_Queues[index].queue->Pop(job)
                                || TryStealJob(job, index) 
                                || TryPopInjectorBatch(job, index);
                    if (!hasMoreJobs) break;
                    job();
                    if (m_ActiveJobCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
                    {
                        std::lock_guard<std::mutex> lock(m_WaitMutex);
                        m_WaitCondition.notify_all();
                    }
                }
            }
        }
    }

    void JobSystem::WaitAll()
    {
        while (m_ActiveJobCount.load(std::memory_order_acquire) > 0)
        {
            Job job;
            bool found = false;

            uint32_t selfIdx = (t_ThreadIndex >= 0) ? static_cast<uint32_t>(t_ThreadIndex) : 0;

            if (t_ThreadIndex >= 0) found = m_Queues[t_ThreadIndex].queue->Pop(job);
            if (!found) found = TryStealJob(job, selfIdx);
            if (!found) found = TryPopInjectorBatch(job, selfIdx);

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
                if (t_ThreadIndex >= 0) std::this_thread::yield();
                else
                {
                    std::unique_lock<std::mutex> lock(m_WaitMutex);
                    m_WaitCondition.wait(lock, [this] 
                    { 
                        return m_ActiveJobCount.load(std::memory_order_acquire) == 0; 
                    });
                }
            }
        }
    }
}