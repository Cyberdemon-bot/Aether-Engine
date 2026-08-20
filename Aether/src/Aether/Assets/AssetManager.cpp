#include "aepch.h"
#include "Aether/Assets/AssetManager.h"

#define UnloadAsset(type) \
    { \
        using TargetPoolType = ResourcePool<Handle<type>, type>; \
        auto& pool = std::get<TargetPoolType>(m_AssetContainers); \
        pool.DestroyResource(Handle<type>::FromBlend(route->handle)); \
    }

namespace Aether {

    void AssetManager::Init()
    {
        m_Handles.reserve(64);
        InitializePools();
        AE_CORE_INFO("AssetManager initialized");
    }

    void AssetManager::Shutdown()
    {
        m_Handles.clear();
        ShutdownPools();
    }

    Handle<Asset> AssetManager::GetHandle(UUID id)
    {
        auto it = m_Handles.find(id);
        if (it == m_Handles.end())
            return {};

        return it->second;
    }

    void AssetManager::Unload(UUID id)
    {
        auto it = m_Handles.find(id);
        if(it == m_Handles.end()) return;
        Unload(it->second);
    }

    void AssetManager::Unload(Handle<Asset> handle)
    {
        auto* route = m_Router.GetResource(handle);
        if (route == nullptr) return;

        switch (route->type)
        {
            case AssetType::Mesh: UnloadAsset(AMesh); break;
            case AssetType::Material: UnloadAsset(AMaterial); break;
            case AssetType::Sheet: UnloadAsset(ASheet); break;
            case AssetType::Skeleton: UnloadAsset(ASkeleton); break;
            case AssetType::Clip: UnloadAsset(AClip); break;
            case AssetType::Script: UnloadAsset(AScript); break;
            case AssetType::Image: UnloadAsset(AImage); break;
            case AssetType::Audio: UnloadAsset(AAudio); break;
            case AssetType::None:
            default:
                AE_CORE_ASSERT(false, "Unknown or invalid asset type in Unload!");
                break;
        }
        m_Router.DestroyResource(handle);
    }

    void AssetManager::InitializePools() 
    {
        m_Router.Init();
        std::apply([](auto&... pools) 
        {
            (pools.Init(), ...);
        }, m_AssetContainers);
    }

    void AssetManager::ShutdownPools()
    {
        m_Router.Shutdown();
        std::apply([](auto&... pools) 
        {
            (pools.Shutdown(), ...);
        }, m_AssetContainers);
    }
}
