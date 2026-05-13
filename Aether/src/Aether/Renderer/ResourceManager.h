#pragma once

#include "Aether/Renderer/Resource.h"
#include "Aether/Container/Handle.h"
#include "Aether/Core/Base.h"
#include <vector>

namespace Aether {

    struct ResourceSlot
    {
        Scope<Resource> asset;
        uint32_t generation = 0;
    };

    class AETHER_API ResourceManager
    {
    public:
        static void Init();
        static void Shutdown();
        static void Unload(Handle<Resource> handle);

        template<typename T, typename... Args>
        static Handle<Resource> CreateResource(Args&&... args)
        {
            auto& instance = GetInstance(); int index;
            static_assert(std::is_base_of_v<Resource, T>, "T must derive from Resource");

            if (!instance.FreeList.empty())
            {
                index = instance.FreeList.back();
                instance.FreeList.pop_back();
            }
            else
            {
                index = instance.m_Resources.size();
                instance.m_Resources.emplace_back();
            }
            ResourceSlot& slot = instance.m_Resources[index];
            slot.asset = T::CreateImpl(std::forward<Args>(args)...);
            Handle<Resource> handle;
            handle.index = index;
            handle.generation = slot.generation;
            return handle;
        }

        template<typename T>
        static T* GetResource(Handle<Resource> handle)
        {
            auto& instance = GetInstance();
            if (handle.index >= instance.m_Resources.size()) return nullptr;
            ResourceSlot& slot = instance.m_Resources[handle.index];
            if (slot.generation != handle.generation) return nullptr;
            return static_cast<T*>(slot.asset.get());
        }
        
    private:
        ResourceManager() = default;
        ResourceManager(const ResourceManager&) = delete;
        ResourceManager& operator=(const ResourceManager&) = delete;
        ResourceManager(ResourceManager&&) = delete;
        ResourceManager& operator=(ResourceManager&&) = delete;

        static ResourceManager& GetInstance();
        std::vector<ResourceSlot> m_Resources;
        std::vector<uint32_t> FreeList;
    };
}