#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Core/UUID.h"
#include "Aether/Core/Base.h"
#include <unordered_map>
#include <vector>

namespace Aether {

    struct AssetSlot
    {
        Scope<Asset> asset;
        int generation = 0;
    };

    class AETHER_API AssetManager
    {
    public:
        static void Init();
        static void Shutdown();
        static void Unload(UUID id);
        static AssetHandle GetHandle(UUID id);

        template<typename T, typename... Args>
        static AssetHandle CreateAsset(UUID id, Args&&... args)
        {
            auto& instance = GetInstance();
            static_assert(std::is_base_of_v<Asset, T>, "T must derive from Asset");
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
            AssetSlot& res = instance.m_Assets[index];
            res.asset = T::CreateImpl(std::forward<Args>(args)...);
            res.asset->id = id;
            AssetHandle handle;
            handle.index = index;
            handle.generation = res.generation;
            instance.m_Handles[id] = handle;
            return handle;
        }

        template<typename T>
        static T* GetAsset(AssetHandle handle)
        {
            auto& instance = GetInstance();
            if (handle.index >= instance.m_Assets.size()) return nullptr;
            AssetSlot& res = instance.m_Assets[handle.index];
            if (res.generation != handle.generation) return nullptr;
            return static_cast<T*>(res.asset.get());
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