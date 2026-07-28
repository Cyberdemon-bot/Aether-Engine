#pragma once

#include <array>
#include <atomic>
#include "Aether/Core/Assert.h"

namespace Aether {

    template<typename T, size_t Capacity>
    class MSPCQueue
    {
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
    public:
        bool Push(T item)
        {
            uint64_t tail = m_Tail.load(std::memory_order_relaxed);
            while (true)
            {
                uint64_t head = m_Head.load(std::memory_order_acquire);
                if (tail - head >= Capacity) return false;
                if (m_Tail.compare_exchange_weak(tail, tail + 1, std::memory_order_relaxed))
                    break;
            }
            
            auto& slot = m_Buffer[tail & (Capacity - 1)];
            slot.data = std::move(item);
            slot.ready.store(true, std::memory_order_release);
            return true;
        }

        bool Pop(T& out)
        {
            uint64_t head = m_Head.load(std::memory_order_relaxed);
            auto& slot = m_Buffer[head & (Capacity - 1)];
            if (!slot.ready.load(std::memory_order_acquire)) return false;
            out = std::move(slot.data); 
            slot.ready.store(false, std::memory_order_relaxed);
            m_Head.fetch_add(1, std::memory_order_release);
            return true;
        }

        template<typename Fn>
        void Drain(Fn&& fn)
        {
            T item;
            while (Pop(item)) fn(std::move(item));
        }
    private:
        struct Slot
        {
            T data;
            std::atomic<bool> ready{false};
        };

        std::array<Slot, Capacity> m_Buffer;
        alignas(64) std::atomic<uint64_t> m_Tail{0};
        alignas(64) std::atomic<uint64_t> m_Head{0};
    };
}