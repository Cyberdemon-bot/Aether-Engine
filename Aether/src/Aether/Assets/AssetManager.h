#pragma once

#include "Aether/Core/UUID.h"
#include "Aether/Core/Base.h"
#include "Aether/Core/Log.h"
#include "Aether/Core/Assert.h"
#include "Aether/Container/ResourcePool.h"
#include "Aether/Assets/Asset.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Assets/Material.h"
#include "Aether/Assets/Rig.h"
#include "Aether/Assets/Script.h"
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


    template<AssetType Type> struct GetTypeFromEnum;
    template<> struct GetTypeFromEnum<AssetType::Mesh> { using Type = Mesh; };
    template<> struct GetTypeFromEnum<AssetType::Material> { using Type = Material; };
    template<> struct GetTypeFromEnum<AssetType::Sheet> { using Type = Sheet; };
    template<> struct GetTypeFromEnum<AssetType::Skeleton> { using Type = Skeleton; };
    template<> struct GetTypeFromEnum<AssetType::Clip> { using Type = Clip; };
    template<> struct GetTypeFromEnum<AssetType::Script> { using Type = Script; };

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
        static void Init();
        static void Shutdown();
        static void Unload(UUID id);
        static void Unload(Handle<Asset> handle);
        static Handle<Asset> GetHandle(UUID id);

        template<typename T, typename... Args>
        static Handle<Asset> CreateAsset(UUID id, Args&&... args)
        {
            auto& instance = GetInstance();
            AE_CORE_ASSERT((std::is_base_of_v<Asset, T>), "T must derive from Asset");
            auto it = instance.m_Handles.find(id); 
            if (it != instance.m_Handles.end())
            {
                AE_CORE_ERROR("ID {0} is already exits in Asset Manager", uint64_t(id));
                return {};
            } 

            using TargetPoolType = ResourcePool<Handle<T>, T>;
            auto& pool = std::get<TargetPoolType>(instance.m_AssetContainers);
            Handle<T> handle = pool.CreateResource(std::forward<Args>(args)...);    
            Handle<Asset> route = instance.m_Router.CreateResource(handle.Blend(), GetAssetType<T>::value);
            instance.m_Handles[id] = route;

            auto* asset = pool.GetResource(handle);
            asset->id = id;
            asset->loaded = true;
            return route;
        }

        template<typename T>
        static T* GetAsset(Handle<Asset> handle)
        {
            auto& instance = GetInstance();
            auto* route = instance.m_Router.GetResource(handle);
            if (!route) return nullptr;
            if (route->type != GetAssetType<T>::value)
            {
                AE_CORE_ERROR("Asset type mismatch in GetAsset!");
                return nullptr;
            }

            using TargetPoolType = ResourcePool<Handle<T>, T>;
            auto& pool = std::get<TargetPoolType>(instance.m_AssetContainers);
            
            return pool.GetResource(Handle<T>::FromBlend(route->handle));
        }

        template<typename T>
        static T* GetAsset(UUID id)
        {
            return GetAsset<T>(GetHandle(id));
        }
        
    private:
        AssetManager() = default;
        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager(AssetManager&&) = delete;
        AssetManager& operator=(AssetManager&&) = delete;
        
        static AssetManager& GetInstance();
        static void InitializePools();
        static void ShutdownPools();
        std::unordered_map<UUID, Handle<Asset>> m_Handles;  
        ResourcePool<Handle<Asset>, Route> m_Router;

        std::tuple<
            ResourcePool<Handle<Mesh>, Mesh>,
            ResourcePool<Handle<Material>, Material>,
            ResourcePool<Handle<Skeleton>, Skeleton>,
            ResourcePool<Handle<Clip>, Clip>,
            ResourcePool<Handle<Sheet>, Sheet>,
            ResourcePool<Handle<Script>, Script>
        > m_AssetContainers;
    };
}