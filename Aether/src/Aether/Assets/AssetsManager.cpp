#include "aepch.h"
#include "Aether/Assets/AssetsManager.h"

namespace Aether {

    AssetsManager& AssetsManager::GetInstance()
    {
        static AssetsManager instance;
        return instance;
    }

    void AssetsManager::Init()
    {
        auto& instance = GetInstance();
        instance.m_Registry.reserve(128);
        instance.m_Resources.reserve(128);
        AE_CORE_INFO("AssetsManager initialized");
    }

    void AssetsManager::Shutdown()
    {
        auto& instance = GetInstance();
        instance.m_Registry.clear();
        instance.m_Resources.clear();
    }

    void AssetsManager::RegisterResource(Ref<Asset> asset, UUID id)
    {
        auto& instance = GetInstance();
        instance.m_Resources[id] = asset;
        instance.m_Registry[id] = asset->GetAssetType();
    }
}
