#pragma once

#include <vector>
#include <concepts>
#include "Aether/Container/Handle.h"

namespace Aether {

    template<typename HandleType, typename DataType>
    class AETHER_API ResourcePool
    {
    public:

        ResourcePool() = default;
        ~ResourcePool() = default;
        
        ResourcePool(ResourcePool&&) = default;
        ResourcePool& operator=(ResourcePool&&) = default;
        struct ResourceSlot
        {
            uint32_t generation = 0;
            DataType asset;
            bool valid = true;

            template<typename... Args>
            ResourceSlot(uint32_t gen, Args&&... args) 
                : generation(gen), asset(std::forward<Args>(args)...) {}
            
            ResourceSlot(uint32_t gen, DataType& res)
                : generation(gen), asset(std::move(res)) {}
        };

        void Init()
        {
            m_Resources.reserve(32);
            m_FreeList.reserve(32);
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

            slot.generation++; m_Size--; slot.valid = false;
            m_FreeList.push_back(handle.index); 
        }

        void Clear()
        {
            m_Size = 0;
            m_Resources.clear();
            m_FreeList.clear();
        }

        uint32_t GetSize() { return m_Size; }
        uint32_t GetLen() { return m_Resources.size(); }

        template<typename... Args>
        HandleType CreateResource(Args&&... args)
        { 
            uint32_t index;
            if (!m_FreeList.empty())
            {
                index = m_FreeList.back();
                m_FreeList.pop_back();
                m_Resources[index].asset = std::move(DataType(std::forward<Args>(args)...));
                m_Resources[index].valid = true;
            }
            else
            {
                index = m_Resources.size();
                m_Resources.emplace_back(0, std::forward<Args>(args)...);
            }

            m_Size++;
            HandleType handle;
            handle.index = index;
            handle.generation = m_Resources[index].generation;
            return handle;
        }

        HandleType SaveResource(DataType& resource)
        {
            uint32_t index;
            if (!m_FreeList.empty())
            {
                index = m_FreeList.back();
                m_FreeList.pop_back();
                m_Resources[index].asset = std::move(resource);
                m_Resources[index].valid = true;
            }
            else
            {
                index = m_Resources.size();
                m_Resources.emplace_back(0, resource);
            }

            m_Size++;
            HandleType handle;
            handle.index = index;
            handle.generation = m_Resources[index].generation;
            return handle;
        }

        HandleType SaveResource(const DataType& resource)
        {
            uint32_t index;
            if (!m_FreeList.empty())
            {
                index = m_FreeList.back();
                m_FreeList.pop_back();
                m_Resources[index].asset = std::move(resource);
                m_Resources[index].valid = true;
            }
            else
            {
                index = m_Resources.size();
                m_Resources.emplace_back(0, resource);
            }

            m_Size++;
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

        bool IsValid(HandleType handle)
        {
            return GetResource(handle) != nullptr;
        }

        template<typename Fn>
        requires std::invocable<Fn, DataType&>
        void Loop(Fn action)
        {
            for (int i = 0; i < m_Resources.size(); i++)
            {
                auto& slot = m_Resources[i];
                if (!slot.valid) continue;
                action(slot.asset);
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