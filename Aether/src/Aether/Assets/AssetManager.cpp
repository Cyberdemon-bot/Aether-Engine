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
        instance.m_Registry.reserve(128);
        instance.m_Resources.reserve(128);
        AE_CORE_INFO("AssetManager initialized");
    }

    void AssetManager::Shutdown()
    {
        auto& instance = GetInstance();
        instance.m_Registry.clear();
        instance.m_Resources.clear();
    }

    void AssetManager::RegisterResource(Ref<Asset> asset, UUID id)
    {
        auto& instance = GetInstance();
        instance.m_Resources[id] = asset;
        instance.m_Registry[id] = asset->GetAssetType();
    }
}
