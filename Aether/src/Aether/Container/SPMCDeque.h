#pragma once

#include <array>
#include <atomic>

namespace Aether {

    template<typename T, size_t Capacity>
    class SPMCDeque
    {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    public:
        void Push(T item)
        {
            int64_t b = m_Bottom.load(std::memory_order_relaxed);
            int64_t t = m_Top.load(std::memory_order_acquire);
            AE_CORE_ASSERT(b - t < static_cast<int64_t>(Capacity), "JobQueue overflow");
            m_Buffer[b % Capacity] = std::move(item);
            std::atomic_thread_fence(std::memory_order_release);
            m_Bottom.store(b + 1, std::memory_order_relaxed);
        }

        bool Pop(T& out)
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

            if (t == b)
            {
                if (!m_Top.compare_exchange_strong(t, t + 1))
                {
                    m_Bottom.store(b + 1, std::memory_order_relaxed);
                    return false;
                }
                out = std::move(m_Buffer[b % Capacity]);
                m_Bottom.store(b + 1, std::memory_order_relaxed);
            }
            else out = std::move(m_Buffer[b % Capacity]);

            return true;
        }

        bool Steal(T& out)
        {
            int64_t t = m_Top.load(std::memory_order_acquire);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            int64_t b = m_Bottom.load(std::memory_order_acquire);

            if (t >= b) return false;
            T item = m_Buffer[t % Capacity];
            if (m_Top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst))
            {
                out = std::move(item);
                return true;
            }
            return false;
        }
    private:
        std::array<T, Capacity> m_Buffer;
        alignas(64) std::atomic<int64_t> m_Top{0};
        alignas(64) std::atomic<int64_t> m_Bottom{0};
    };
}