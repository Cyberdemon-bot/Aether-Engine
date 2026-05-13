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
        instance.m_Handles.reserve(64);
        instance.m_Assets.reserve(64);
        AE_CORE_INFO("AssetManager initialized");
    }

    void AssetManager::Shutdown()
    {
        auto& instance = GetInstance();
        instance.m_Handles.clear();
        instance.m_Assets.clear();
    }

    Handle<Asset> AssetManager::GetHandle(UUID id)
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
        Handle<Asset> handle = it->second;
        if (handle.index >= instance.m_Assets.size()) return;
        AssetSlot& slot = instance.m_Assets[handle.index];
        if (slot.generation != handle.generation) return;

        slot.asset.reset();
        slot.loaded = false;
        slot.generation++;
        instance.FreeList.push_back(handle.index);
        instance.m_Handles.erase(it);
    }

    Handle<Asset> AssetManager::RequestAssetSlot(UUID id)
    {
        auto& instance = GetInstance();
        auto it = instance.m_Handles.find(id); int index;
        if (it != instance.m_Handles.end())
        {
            AE_CORE_ERROR("ID {0} is already exits in Asset Manager", uint64_t(id));
            return {};
        } 

        if (!instance.FreeList.empty())
        {
            index = instance.FreeList.back();
            instance.FreeList.pop_back();
        }
        else
        {
            index = instance.m_Assets.size();
            instance.m_Assets.emplace_back();
        }
        AssetSlot& slot = instance.m_Assets[index]; slot.id = id;
        Handle<Asset> handle;
        handle.index = index;
        handle.generation = slot.generation;
        instance.m_Handles[id] = handle;
        return handle;
    }
}
