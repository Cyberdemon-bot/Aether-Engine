#pragma once

#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>

namespace Aether {

    template<typename T, size_t Capacity>
    class MPSCQueue
    {
        static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");

    public:
        MPSCQueue()
        {
            for (size_t i = 0; i < Capacity; ++i)
            {
                Slot* slot = GetSlot(i);
                ::new (static_cast<void*>(&slot->ready)) std::atomic<bool>(false);
            }
        }

        ~MPSCQueue()
        {
            T item;
            while (Pop(item)) {}

            for (size_t i = 0; i < Capacity; ++i)
            {
                Slot* slot = GetSlot(i);
                std::destroy_at(&slot->ready);
            }
        }

        MPSCQueue(const MPSCQueue&) = delete;
        MPSCQueue& operator=(const MPSCQueue&) = delete;
        MPSCQueue(MPSCQueue&&) = delete;
        MPSCQueue& operator=(MPSCQueue&&) = delete;

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

            Slot* slot = GetSlot(tail);
            T* ptr = reinterpret_cast<T*>(slot->storage);
            ::new (static_cast<void*>(ptr)) T(std::move(item));
            slot->ready.store(true, std::memory_order_release);
            return true;
        }

        bool Pop(T& out)
        {
            uint64_t head = m_Head.load(std::memory_order_relaxed);
            Slot* slot = GetSlot(head);
            if (!slot->ready.load(std::memory_order_acquire)) return false;

            T* ptr = reinterpret_cast<T*>(slot->storage);
            out = std::move(*ptr);
            std::destroy_at(ptr);

            slot->ready.store(false, std::memory_order_relaxed);
            m_Head.fetch_add(1, std::memory_order_release);
            return true;
        }

        template<typename Fn>
        void Drain(Fn&& fn)
        {
            T item;
            while (Pop(item)) fn(std::move(item));
        }

        size_t Size()
        {
            return Capacity;
        }

    private:
        struct Slot
        {
            alignas(alignof(T)) std::byte storage[sizeof(T)];
            std::atomic<bool> ready;
        };

        Slot* GetSlot(uint64_t index)
        {
            return reinterpret_cast<Slot*>(&m_Buffer[(index & (Capacity - 1)) * sizeof(Slot)]);
        }

        alignas(alignof(Slot)) std::byte m_Buffer[Capacity * sizeof(Slot)];
        alignas(64) std::atomic<uint64_t> m_Tail{0};
        alignas(64) std::atomic<uint64_t> m_Head{0};
    };
}