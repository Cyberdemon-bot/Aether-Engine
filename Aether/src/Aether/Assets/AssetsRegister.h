#pragma once
#include <string> 
#include <type_traits>
#include <unordered_map>
#include "Aether/Core/UUID.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Assets/RegisterInfo.h"
namespace Aether {
    class AssetsRegister
    {
    public:
        void Init();
        void Shutdown();
        std::string Get(UUID key);
        UUID Register(std::string_view name, UUID id)
        {
            m_Map[id] = std::string(name);
            return id;
        }

        template<typename T, typename I>
        UUID Register(const I& info, std::string_view name, UUID id)
        {
            auto* manager = ServiceManager::GetService<AssetManager>();

            if constexpr (std::is_same_v<T, Mesh> && std::is_same_v<I, AMeshCreateInfo>) MeshAssembler(id, manager, info);
            else if constexpr (std::is_same_v<T, Image> && std::is_same_v<I, AImageCreateInfo>) ImageAssembler(id, manager, info);
            else if constexpr (std::is_same_v<T, Material> && std::is_same_v<I, AMaterialCreateInfo>) MaterialAssembler(id, manager, info);
            else if constexpr (std::is_same_v<T, Skeleton> && std::is_same_v<I, ASkeletonCreateInfo>) SkeletonAssembler(id, manager, info);
            else if constexpr (std::is_same_v<T, Clip> && std::is_same_v<I, AClipCreateInfo>) ClipAssembler(id, manager, info);
            else if constexpr (std::is_same_v<T, Sheet> && std::is_same_v<I, ASheetCreateInfo>) SheetAssembler(id, manager, info);
            else AE_CORE_ERROR("cannot register this type");
            
            return Register(name, id);
        }
    private:
        std::unordered_map<UUID, std::string> m_Map;
        
        void MeshAssembler(UUID id, AssetManager* manager, const AMeshCreateInfo& info);
        void ImageAssembler(UUID id, AssetManager* manager, const AImageCreateInfo& info);
        void MaterialAssembler(UUID id, AssetManager* manager, const AMaterialCreateInfo& info);
        void SkeletonAssembler(UUID id, AssetManager* manager, const ASkeletonCreateInfo& info);
        void ClipAssembler(UUID id, AssetManager* manager, const AClipCreateInfo& info);
        void SheetAssembler(UUID id, AssetManager* manager, const ASheetCreateInfo& info);
    };
}