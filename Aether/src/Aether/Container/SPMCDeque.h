#pragma once

#include <atomic>
#include <memory>
#include <cstddef>
#include <utility>

namespace Aether {

    template<typename T, size_t Capacity>
    class SPMCDeque
    {
        static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    public:
        SPMCDeque() = default;

        ~SPMCDeque()
        {
            int64_t t = m_Top.load(std::memory_order_relaxed);
            int64_t b = m_Bottom.load(std::memory_order_relaxed);
            for (int64_t i = t; i < b; ++i)
            {
                std::destroy_at(GetSlot(i));
            }
        }

        SPMCDeque(const SPMCDeque&) = delete;
        SPMCDeque& operator=(const SPMCDeque&) = delete;
        SPMCDeque(SPMCDeque&&) = delete;
        SPMCDeque& operator=(SPMCDeque&&) = delete;

        bool Push(T item)
        {
            int64_t b = m_Bottom.load(std::memory_order_relaxed);
            int64_t t = m_Top.load(std::memory_order_acquire);
            if (b - t >= static_cast<int64_t>(Capacity)) return false;

            ::new (static_cast<void*>(GetSlot(b))) T(std::move(item));
            m_Bottom.store(b + 1, std::memory_order_release);
            return true;
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

            T* slot = GetSlot(b);

            if (t == b)
            {
                if (!m_Top.compare_exchange_strong(t, t + 1))
                {
                    m_Bottom.store(b + 1, std::memory_order_relaxed);
                    return false;
                }
                out = std::move(*slot);
                std::destroy_at(slot);
                m_Bottom.store(b + 1, std::memory_order_relaxed);
            }
            else 
            {
                out = std::move(*slot);
                std::destroy_at(slot);
            }

            return true;
        }

        bool Steal(T& out)
        {
            int64_t t = m_Top.load(std::memory_order_acquire);
            std::atomic_thread_fence(std::memory_order_seq_cst);
            int64_t b = m_Bottom.load(std::memory_order_acquire);

            if (t >= b) return false;
            if (m_Top.compare_exchange_strong(t, t + 1, std::memory_order_seq_cst, std::memory_order_relaxed))
            {
                T* slot = GetSlot(t);
                out = std::move(*slot);
                std::destroy_at(slot);
                return true;
            }
            return false;
        }

        size_t Size()
        {
            return Capacity;
        }

    private:
        T* GetSlot(int64_t index)
        {
            return reinterpret_cast<T*>(&m_Buffer[(index & (Capacity - 1)) * sizeof(T)]);
        }

        alignas(alignof(T)) std::byte m_Buffer[Capacity * sizeof(T)];
        alignas(64) std::atomic<int64_t> m_Top{0};
        alignas(64) std::atomic<int64_t> m_Bottom{0};
    };
}