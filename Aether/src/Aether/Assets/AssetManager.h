#pragma once

#include "Aether/Core/UUID.h"
#include "Aether/Core/Base.h"
#include "Aether/Core/Log.h"
#include "Aether/Core/Assert.h"
#include "Aether/Assets/Asset.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Assets/Material.h"
#include "Aether/Assets/Rig.h"
#include "Aether/Assets/Media.h"
#include "Aether/Container/ResourcePool.h"
#include <unordered_map>
#include <tuple>

namespace Aether {

    template<typename T> struct GetAssetType;
    template<> struct GetAssetType<Mesh> { static constexpr AssetType value = AssetType::Mesh; };
    template<> struct GetAssetType<Material> { static constexpr AssetType value = AssetType::Material; };
    template<> struct GetAssetType<Sheet> { static constexpr AssetType value = AssetType::Sheet; };
    template<> struct GetAssetType<Skeleton> { static constexpr AssetType value = AssetType::Skeleton; };
    template<> struct GetAssetType<Clip> { static constexpr AssetType value = AssetType::Clip; };
    template<> struct GetAssetType<Script> { static constexpr AssetType value = AssetType::Script; };
    template<> struct GetAssetType<Image> { static constexpr AssetType value = AssetType::Image; };
    template<> struct GetAssetType<Audio> { static constexpr AssetType value = AssetType::Audio; };

    struct Route
    {
        Route(uint64_t h, AssetType t)
        {
            handle = h;
            type = t;
        }
        uint64_t handle = 0;
        AssetType type = AssetType::None;
    };

    class AETHER_API AssetManager
    {
    public:
        void Init();
        void Shutdown();
        void Unload(UUID id);
        void Unload(Handle<Asset> handle);
        Handle<Asset> GetHandle(UUID id);

        template<typename T, typename... Args>
        Handle<Asset> CreateAsset(UUID id, Args&&... args)
        {
            AE_CORE_ASSERT((std::is_base_of_v<Asset, T>), "T must derive from Asset");
            auto it = m_Handles.find(id); 
            if (it != m_Handles.end())
            {
                AE_CORE_ERROR("ID {0} is already exits in Asset Manager", uint64_t(id));
                return {};
            } 

            using TargetPoolType = ResourcePool<Handle<T>, T>;
            auto& pool = std::get<TargetPoolType>(m_AssetContainers);
            Handle<T> handle = pool.CreateResource(std::forward<Args>(args)...);    
            Handle<Asset> route = m_Router.CreateResource(handle.Blend(), GetAssetType<T>::value);
            m_Handles[id] = route;

            auto* asset = pool.GetResource(handle);
            asset->id = id;
            asset->loaded = true;
            return route;
        }

        template<typename T>
        T* GetAsset(Handle<Asset> handle)
        {
            auto* route = m_Router.GetResource(handle);
            if (!route) return nullptr;
            if (route->type != GetAssetType<T>::value)
            {
                AE_CORE_ERROR("Asset type mismatch in GetAsset!");
                return nullptr;
            }

            using TargetPoolType = ResourcePool<Handle<T>, T>;
            auto& pool = std::get<TargetPoolType>(m_AssetContainers);
            
            return pool.GetResource(Handle<T>::FromBlend(route->handle));
        }

        template<typename T>
        T* GetAsset(UUID id)
        {
            return GetAsset<T>(GetHandle(id));
        }
        
    private:
        void InitializePools();
        void ShutdownPools();
        std::unordered_map<UUID, Handle<Asset>> m_Handles;  
        ResourcePool<Handle<Asset>, Route> m_Router;

        std::tuple<
            ResourcePool<Handle<Mesh>, Mesh>,
            ResourcePool<Handle<Material>, Material>,
            ResourcePool<Handle<Skeleton>, Skeleton>,
            ResourcePool<Handle<Clip>, Clip>,
            ResourcePool<Handle<Sheet>, Sheet>,
            ResourcePool<Handle<Script>, Script>,
            ResourcePool<Handle<Image>, Image>,
            ResourcePool<Handle<Audio>, Audio>
        > m_AssetContainers;
    };
}