#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/Log.h"
#include <vector>

namespace Aether {

    template<typename Tag>
    struct Handle
    {
        uint32_t index = UINT32_MAX;
        uint32_t generation = 0;
        bool IsValid() const { return index != UINT32_MAX; }
        static Handle MakeInvalid() { return {UINT32_MAX, 0}; }
        uint64_t Blend() const { return ((uint64_t)index << 32) | generation; }
    };

    template<typename HandleType, typename DataType>
    class AETHER_API ResourcePool
    {
    public:
        struct ResourceSlot
        {
            DataType asset;
            uint32_t generation = 0;

            template<typename... Args>
            ResourceSlot(uint32_t gen, Args&&... args) 
                : generation(gen), asset(std::forward<Args>(args)...) {}
        };

        ResourcePool()
        {
            m_Resources.reserve(128);
            AE_CORE_INFO("ResourcePool {0} initialized", AE_GET_CHAR(DataType));
        }

        ~ResourcePool()
        {
            Clear();
            AE_CORE_INFO("ResourcePool {0} shutdowned", AE_GET_CHAR(DataType));
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
                m_Resources[index].asset = DataType(std::forward<Args>(args)...);
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

        std::vector<ResourceSlot> m_Resources;
        std::vector<uint32_t> FreeList;
    };
}