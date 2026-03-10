#include "aepch.h"
#include "Aether/Assets/AssetManager.h"

namespace Aether {

    AssetManager& AssetManager::GetInstance()
    {
        static AssetManager instance;
        return instance;
    }

    void AssetManager::Init()
    {
        auto& instance = GetInstance();
        instance.m_Handles.reserve(128);
        instance.m_Assets.reserve(128);
        AE_CORE_INFO("AssetManager initialized");
    }

    void AssetManager::Shutdown()
    {
        auto& instance = GetInstance();
        instance.m_Handles.clear();
        instance.m_Assets.clear();
    }

    AssetHandle AssetManager::GetHandle(UUID id)
    {
        auto& instance = GetInstance();
        auto it = instance.m_Handles.find(id);
        if (it == instance.m_Handles.end())
            return {};

        return it->second;
    }

    void AssetManager::Unload(UUID id)
    {
        auto& instance = GetInstance();
        auto it = instance.m_Handles.find(id);
        if (it == instance.m_Handles.end()) return;
        AssetHandle handle = it->second;
        if (handle.index >= instance.m_Assets.size()) return;
        AssetSlot& res = instance.m_Assets[handle.index];
        if (res.generation != handle.generation) return;

        res.asset.reset();
        res.generation++;
        instance.FreeList.push_back(handle.index);
        instance.m_Handles.erase(it);
    }
}
