#include "aepch.h"
#include "Aether/Assets/AssetManager.h"

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
            case AssetType::Mesh:
            {
                using TargetPoolType = ResourcePool<Handle<Mesh>, Mesh>;
                auto& pool = std::get<TargetPoolType>(m_AssetContainers);
                pool.DestroyResource(Handle<Mesh>::FromBlend(route->handle));
                break;
            }
            case AssetType::Material:
            {
                using TargetPoolType = ResourcePool<Handle<Material>, Material>;
                auto& pool = std::get<TargetPoolType>(m_AssetContainers);
                pool.DestroyResource(Handle<Material>::FromBlend(route->handle));
                break;
            }
            case AssetType::Sheet:
            {
                using TargetPoolType = ResourcePool<Handle<Sheet>, Sheet>;
                auto& pool = std::get<TargetPoolType>(m_AssetContainers);
                pool.DestroyResource(Handle<Sheet>::FromBlend(route->handle));
                break;
            }
            case AssetType::Skeleton:
            {
                using TargetPoolType = ResourcePool<Handle<Skeleton>, Skeleton>;
                auto& pool = std::get<TargetPoolType>(m_AssetContainers);
                pool.DestroyResource(Handle<Skeleton>::FromBlend(route->handle));
                break;
            }
            case AssetType::Clip:
            {
                using TargetPoolType = ResourcePool<Handle<Clip>, Clip>;
                auto& pool = std::get<TargetPoolType>(m_AssetContainers);
                pool.DestroyResource(Handle<Clip>::FromBlend(route->handle));
                break;
            }
            case AssetType::Script:
            {
                using TargetPoolType = ResourcePool<Handle<Script>, Script>;
                auto& pool = std::get<TargetPoolType>(m_AssetContainers);
                pool.DestroyResource(Handle<Script>::FromBlend(route->handle));
                break;
            }
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
