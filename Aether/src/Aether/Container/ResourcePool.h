#pragma once

#include <vector>
#include <memory>
#include <optional> // Thêm include này
#include <utility>  // Thêm include này
#include <concepts>
#include "Aether/Container/Handle.h"

namespace Aether {

    template<typename HandleType, typename DataType>
    class AETHER_API ResourcePool
    {
    public:
        ResourcePool() = default;
        ~ResourcePool() = default;
        
        ResourcePool(ResourcePool&&) noexcept = default;
        ResourcePool& operator=(ResourcePool&&) noexcept = default;
        struct ResourceSlot
        {
            uint32_t generation = 0;
            std::optional<DataType> asset; 
        };

        void Init()
        {
            m_Resources.reserve(32);
            m_FreeList.reserve(32);
        }

        void Shutdown()
        {
            Clear();
            m_Resources.shrink_to_fit();
            m_FreeList.shrink_to_fit();
        }

        void DestroyResource(HandleType handle)
        {
            if (handle.index >= m_Resources.size()) return;
            ResourceSlot& slot = m_Resources[handle.index];
            if (slot.generation != handle.generation || !slot.asset.has_value()) return;

            slot.asset.reset();
            slot.generation++; 
            m_Size--; 
            m_FreeList.push_back(handle.index); 
        }

        void Clear()
        {
            m_Size = 0;
            m_Resources.clear();
            m_FreeList.clear();
        }

        uint32_t GetSize() const { return m_Size; }
        uint32_t GetLen() const { return static_cast<uint32_t>(m_Resources.size()); }

        template<typename... Args>
        HandleType CreateResource(Args&&... args)
        { 
            uint32_t index;
            if (!m_FreeList.empty())
            {
                index = m_FreeList.back();
                m_FreeList.pop_back();
            }
            else
            {
                index = static_cast<uint32_t>(m_Resources.size());
                m_Resources.emplace_back(); 
            }

            ResourceSlot& slot = m_Resources[index];
            slot.asset.emplace(std::forward<Args>(args)...);

            m_Size++;
            return HandleType{ index, slot.generation };
        }

        HandleType SaveResource(const DataType& resource)
        {
            return CreateResource(resource);
        }

        HandleType SaveResource(DataType&& resource)
        {
            return CreateResource(std::move(resource));
        }

        DataType* GetResource(HandleType handle)
        {
            if (handle.index >= m_Resources.size()) return nullptr;
            ResourceSlot& slot = m_Resources[handle.index]; 
            if (slot.generation != handle.generation || !slot.asset.has_value()) return nullptr;
            return &slot.asset.value();
        }

        const DataType* GetResource(HandleType handle) const
        {
            if (handle.index >= m_Resources.size()) return nullptr;
            const ResourceSlot& slot = m_Resources[handle.index];
            if (slot.generation != handle.generation || !slot.asset.has_value()) return nullptr;
            return &slot.asset.value();
        }

        bool IsValid(HandleType handle) const
        {
            return GetResource(handle) != nullptr;
        }

        template<typename Fn>
        requires std::invocable<Fn, DataType&>
        void Loop(Fn action)
        {
            for (size_t i = 0; i < m_Resources.size(); ++i)
            {
                auto& slot = m_Resources[i];
                if (!slot.asset.has_value()) continue;
                
                action(*slot.asset); 
            }
        }
        
    private:
        ResourcePool& operator=(const ResourcePool&) = delete;
        ResourcePool(const ResourcePool&) = delete;

        std::vector<ResourceSlot> m_Resources;
        std::vector<uint32_t> m_FreeList;
        uint32_t m_Size = 0;
    };
}