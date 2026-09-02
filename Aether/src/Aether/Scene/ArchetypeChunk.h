#pragma once

#include <span>
#include <array>
#include <cstddef>
#include <algorithm>
#include "Aether/Scene/TComponentInfo.h"
#include "Aether/Core/Log.h" 

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

    template<size_t ChunkSizeBytes, size_t MaxComponents = 32>
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
}