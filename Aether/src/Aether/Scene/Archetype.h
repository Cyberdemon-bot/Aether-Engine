#pragma once

#include <span>
#include <array>
#include <cstddef>
#include <algorithm>
#include "Aether/Core/Log.h"
#include "Aether/Core/Base.h"
#include "Aether/Scene/TComponentInfo.h"

namespace Aether {
    namespace Utils
    {
        constexpr size_t AlignUp(size_t address, size_t alignment) 
        {
            if (alignment == 0) return address;
            return (address + (alignment - 1)) & ~(alignment - 1);
        }
    }

    template<size_t MaxComponents = 32>
    struct ArchetypeLayout
    {
        size_t capacity = 0; // num of element per chunk
        size_t total_bytes = 0; // bytes per chunk
        size_t field_count = 0;
        std::array<size_t, MaxComponents> offsets{};
        std::array<TComponentInfo, MaxComponents> components{}; 
    };

    template<size_t ChunkSizeBytes, size_t MaxComponents>
    class ArchetypeLayoutCalculator
    {
    public:
        static ArchetypeLayout<MaxComponents> ComputeLayout(std::span<const TComponentInfo> components)
        {
            ArchetypeLayout<MaxComponents> layout{};

            if (ChunkSizeBytes == 0 || components.empty()) return layout;

            if (components.size() > MaxComponents)
            {
                AE_CORE_ERROR("[ArchetypeLayoutCalculator] Component count ({0}) exceeds MaxComponents ({1})!", components.size(), MaxComponents);
                return layout;
            }

            layout.field_count = components.size();
            for (size_t i = 0; i < layout.field_count; ++i) layout.components[i] = components[i];

            std::sort(layout.components.begin(), layout.components.begin() + layout.field_count,
                [](const TComponentInfo& a, const TComponentInfo& b) 
                {
                    return a.alignment > b.alignment;
                });

            size_t size_per_element = 0;
            for (size_t i = 0; i < layout.field_count; ++i) size_per_element += layout.components[i].size;
            if (size_per_element == 0) return layout;

            size_t low = 0;
            size_t high = ChunkSizeBytes / size_per_element;
            size_t capacity = 0;

            while (low <= high)
            {
                size_t mid = low + (high - low) / 2;

                if (TryLayout(mid, layout))
                {
                    capacity = mid;
                    low = mid + 1; 
                }
                else 
                {
                    if (mid == 0) break;
                    high = mid - 1;
                }
            }

            ImplementLayout(capacity, layout);
            return layout;
        }

    private:
        static bool TryLayout(size_t capacity, const ArchetypeLayout<MaxComponents>& layout)
         {
            size_t curr_offset = 0;

            for (size_t i = 0; i < layout.field_count; ++i)
            {
                curr_offset = Utils::AlignUp(curr_offset, layout.components[i].alignment);
                size_t field_size = layout.components[i].size * capacity;
                curr_offset += field_size;

                if (curr_offset > ChunkSizeBytes) return false;
            }
            
            return true;
        }
        static void ImplementLayout(size_t capacity, ArchetypeLayout<MaxComponents>& layout)
        {
            size_t curr_offset = 0;

            for (size_t i = 0; i < layout.field_count; ++i)
            {
                curr_offset = Utils::AlignUp(curr_offset, layout.components[i].alignment);
                layout.offsets[i] = curr_offset;
                size_t field_size = layout.components[i].size * capacity;
                curr_offset += field_size;
            }

            layout.total_bytes = curr_offset;
            layout.capacity = capacity;
        }
    };

    template<size_t ChunkSizeBytes>
    struct ArchetypeChunk
    {
        alignas(64) std::byte buffer[ChunkSizeBytes];
    };

    template<size_t ChunkSizeBytes = 16384, size_t MaxComponents = 32>
    class Archetype
    {

    public:
        using ChunkType = ArchetypeChunk<ChunkSizeBytes>;
        using LayoutType = ArchetypeLayout<MaxComponents>;
        struct SwapResult
        {
            bool swapped = false;          
            uint32_t movedElementIdx = 0; 
        };

        Archetype(std::span<const TComponentInfo> components)
        {
            m_Layout = ArchetypeLayoutCalculator<ChunkSizeBytes, MaxComponents>::ComputeLayout(components);
        }

        bool CreateElement(uint32_t& newElement)
        {
            if (m_Layout.capacity == 0)
            {
                AE_CORE_ERROR("[Archetype] Cannot create element: Layout capacity is 0!");
                return false;
            }
            uint32_t globalIdx = m_Count++;
            uint32_t capacity = static_cast<uint32_t>(m_Layout.capacity);
            uint32_t chunkIdx = globalIdx / capacity;
            uint32_t localIdx = globalIdx % capacity;

            if (chunkIdx >= m_Chunks.size()) m_Chunks.emplace_back(CreateScope<ChunkType>());
            ChunkType* chunk = m_Chunks[chunkIdx].get();

            for (size_t i = 0; i < m_Layout.field_count; ++i)
            {
                const TComponentInfo& info = m_Layout.components[i];
                std::byte* componentArray = chunk->buffer + m_Layout.offsets[i];
                std::byte* target = componentArray + (localIdx * info.size);

                if (info.ctor) info.ctor(target, 1);
            }

            newElement = globalIdx;
            return true;
        }

        bool DestroyElement(uint32_t globalIdx, SwapResult& out)
        {
            if (m_Layout.capacity == 0 || globalIdx >= m_Count) return false;
            uint32_t capacity = static_cast<uint32_t>(m_Layout.capacity);
            uint32_t lastIdx = m_Count - 1;

            uint32_t t_chunkIdx = globalIdx / capacity;
            uint32_t t_localIdx = globalIdx % capacity;
            uint32_t l_chunkIdx = lastIdx / capacity;
            uint32_t l_localIdx = lastIdx % capacity;

            ChunkType* t_chunk = m_Chunks[t_chunkIdx].get();
            ChunkType* l_chunk = m_Chunks[l_chunkIdx].get();

            out.swapped = (globalIdx != lastIdx);
            out.movedElementIdx = lastIdx;

            if (out.swapped)
            {
                for (size_t i = 0; i < m_Layout.field_count; ++i)
                {
                    const TComponentInfo& info = m_Layout.components[i];
                    std::byte* t_components = t_chunk->buffer + m_Layout.offsets[i];
                    std::byte* l_components = l_chunk->buffer + m_Layout.offsets[i];
                    std::byte* t_elem = t_components + (t_localIdx * info.size);
                    std::byte* l_elem = l_components + (l_localIdx * info.size);
                    
                    if (info.move) info.move(t_elem, l_elem, 1);
                    else std::memcpy(t_elem, l_elem, info.size);

                    if (info.dtor) info.dtor(l_elem, 1);
                }
            }
            else
            {
                for (size_t i = 0; i < m_Layout.field_count; ++i)
                {
                    const TComponentInfo& info = m_Layout.components[i];
                    std::byte* l_components = t_chunk->buffer + m_Layout.offsets[i];
                    std::byte* l_elem = l_components + (l_localIdx * info.size);
                    if (info.dtor) info.dtor(l_elem, 1);
                }
            }
            --m_Count;
            if (m_Count % capacity == 0 && !m_Chunks.empty()) m_Chunks.pop_back();
            return true;
        }
    private:
        uint32_t m_Count = 0;
        LayoutType m_Layout;
        std::vector<Scope<ChunkType>> m_Chunks;
    };
}