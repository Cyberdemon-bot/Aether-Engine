#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Core/UUID.h"
#include "Aether/Core/Base.h"
#include <unordered_map>
#include <vector>

namespace Aether {

    struct AssetSlot
    {
        Scope<Asset> asset = nullptr;
        UUID id = UUID(0);
        bool loaded = false;
        int generation = 0;
    };

    class AETHER_API AssetManager
    {
    public:
        static void Init();
        static void Shutdown();
        static void Unload(UUID id);
        static AssetHandle GetHandle(UUID id);
        static AssetHandle RequestAssetSlot(UUID id);

        template<typename T, typename... Args>
        static AssetHandle CreateAsset(UUID id, Args&&... args)
        {
            auto& instance = GetInstance();
            AE_CORE_ASSERT(std::is_base_of_v<Asset, T>, "T must derive from Asset");
            AssetHandle handle = RequestAssetSlot(id);
            AssetSlot& slot = instance.m_Assets[handle.index];
            slot.asset = T::CreateImpl(std::forward<Args>(args)...);
            slot.asset->id = id;
            slot.loaded = true;
            return handle;
        }

        template<typename T, typename... Args>
        static void CommitAsset(AssetHandle handle, Args&&... args)
        {
            auto& instance = GetInstance();
            AE_CORE_ASSERT(std::is_base_of_v<Asset, T>, "T must derive from Asset");
            if (handle.index >= instance.m_Assets.size()) return;
            AssetSlot& slot = instance.m_Assets[handle.index];
            if (slot.generation != handle.generation) return;
            if (slot.loaded == true) AE_CORE_WARN("Overriding loaded Asset!");
            slot.asset = T::CreateImpl(std::forward<Args>(args)...);
            slot.asset->id = slot.id;
            slot.loaded = true;
        }

        template<typename T>
        static T* GetAsset(AssetHandle handle)
        {
            auto& instance = GetInstance();
            if (handle.index >= instance.m_Assets.size()) return nullptr;
            AssetSlot& slot = instance.m_Assets[handle.index];
            if (slot.generation != handle.generation) return nullptr;
            return static_cast<T*>(slot.asset.get());
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
        std::unordered_map<UUID, AssetHandle> m_Handles;
        std::vector<AssetSlot> m_Assets;
        std::vector<uint32_t> FreeList;
    };
}