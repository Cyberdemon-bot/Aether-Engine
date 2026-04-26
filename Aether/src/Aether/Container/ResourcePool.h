#pragma once

#include "Aether/Core/Log.h"
#include "Handle.h"
#include <vector>

namespace Aether {

    template<typename HandleType, typename DataType>
    class AETHER_API ResourcePool
    {
    public:

        ResourcePool() = default;
        ~ResourcePool() = default;
        struct ResourceSlot
        {
            DataType asset;
            uint32_t generation = 0;

            template<typename... Args>
            ResourceSlot(uint32_t gen, Args&&... args) 
                : generation(gen), asset(std::forward<Args>(args)...) {}
        };

        void Init()
        {
            m_Resources.reserve(64);
        }

        void Shutdown()
        {
            Clear();
        }

        void DestroyResource(HandleType handle)
        {
            if (handle.index >= m_Resources.size()) return;
            ResourceSlot& slot = m_Resources[handle.index];
            if (slot.generation != handle.generation) return;

            slot.generation++;
            FreeList.push_back(handle.index);
        }

        void Clear()
        {
            m_Resources.clear();
            FreeList.clear();
        }

        uint32_t GetSize() { return m_Resources.size(); }

        typename std::vector<ResourceSlot>::iterator Begin() { return m_Resources.begin(); }
        typename std::vector<ResourceSlot>::iterator End() { return m_Resources.end(); }

        template<typename... Args>
        HandleType CreateResource(Args&&... args)
        { 
            uint32_t index;
            if (!FreeList.empty())
            {
                index = FreeList.back();
                FreeList.pop_back();
                m_Resources[index].asset = std::move(DataType(std::forward<Args>(args)...));
            }
            else
            {
                index = m_Resources.size();
                m_Resources.emplace_back(0, std::forward<Args>(args)...);
            }

            HandleType handle;
            handle.index = index;
            handle.generation = m_Resources[index].generation;
            return handle;
        }

        DataType* GetResource(HandleType handle)
        {
            if (handle.index >= m_Resources.size()) return nullptr;
            
            ResourceSlot& slot = m_Resources[handle.index]; 
            
            if (slot.generation != handle.generation) return nullptr;
            return &slot.asset;
        }

        const DataType* GetResource(HandleType handle) const
        {
            if (handle.index >= m_Resources.size()) return nullptr;
            const ResourceSlot& slot = m_Resources[handle.index];
            if (slot.generation != handle.generation) return nullptr;
            return &slot.asset;
        }
        
    private:
        ResourcePool& operator=(const ResourcePool&) = delete;
        ResourcePool(const ResourcePool&) = delete;

        std::vector<ResourceSlot> m_Resources;
        std::vector<uint32_t> FreeList;
    };
}