#pragma once

#include "Aether/Core/Log.h"
#include "Aether/Container/Handle.h"
#include <vector>

namespace Aether {

    template<typename HandleType, typename DataType>
    class AETHER_API AbstractResourcePool
    {
    public:
        AbstractResourcePool() = default;
        ~AbstractResourcePool() = default;

        struct ResourceSlot
        {
            Scope<DataType> asset;
            uint32_t generation = 0;

            template<typename... Args>
            ResourceSlot(uint32_t gen, Args&&... args) 
                : generation(gen), asset(DataType::CreateImpl(std::forward<Args>(args)...)) {}
        };

        void Init()
        {
            m_Resources.reserve(128);
            AE_CORE_INFO("AbstractResourcePool initialized");
        }

        void Shutdown()
        {
            Clear();
            AE_CORE_INFO("AbstractResourcePool shutdowned");
        }

        void DestroyResource(HandleType handle)
        {
            if (handle.index >= m_Resources.size()) return;
            ResourceSlot& slot = m_Resources[handle.index];
            if (slot.generation != handle.generation) return;

            slot.generation++;
            slot.asset.reset();
            FreeList.push_back(handle.index);
        }

        void Clear()
        {
            for (auto& slot : m_Resources) slot.asset.reset();
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
                m_Resources[index].asset = std::move(DataType::CreateImpl(std::forward<Args>(args)...));
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
            return slot.asset.get();
        }

        const DataType* GetResource(HandleType handle) const
        {
            if (handle.index >= m_Resources.size()) return nullptr;
            const ResourceSlot& slot = m_Resources[handle.index];
            if (slot.generation != handle.generation) return nullptr;
            return &slot.asset;
        }
        
    private:
        AbstractResourcePool& operator=(const AbstractResourcePool&) = delete;
        AbstractResourcePool(const AbstractResourcePool&) = delete;

        std::vector<ResourceSlot> m_Resources;
        std::vector<uint32_t> FreeList;
    };
}