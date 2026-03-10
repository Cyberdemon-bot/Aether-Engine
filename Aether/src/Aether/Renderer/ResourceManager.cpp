#include "aepch.h"
#include "Aether/Renderer/ResourceManager.h"

namespace Aether {

    ResourceManager& ResourceManager::GetInstance()
    {
        static ResourceManager instance;
        return instance;
    }

    void ResourceManager::Init()
    {
        auto& instance = GetInstance();
        instance.m_Resources.reserve(128);
        AE_CORE_INFO("ResourceManager initialized");
    }

    void ResourceManager::Shutdown()
    {
        auto& instance = GetInstance();
        instance.m_Resources.clear();
    }

    void ResourceManager::Unload(ResourceHandle handle)
    {
        auto& instance = GetInstance();
        if (handle.index >= instance.m_Resources.size()) return;
        ResourceSlot& res = instance.m_Resources[handle.index];
        if (res.generation != handle.generation) return;

        res.asset.reset();
        res.generation++;
        instance.FreeList.push_back(handle.index);
    }
}
