#pragma once
#include <string> 
#include <type_traits>
#include <unordered_map>
#include "Aether/Core/UUID.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Assets/RegisterInfo.h"

#define REGISTER_TYPE(Type, InfoType, AssemblerFunc) \
    if constexpr (std::is_same_v<T, Type> && std::is_same_v<I, InfoType>) \
        AssemblerFunc(id, manager, info);

namespace Aether {
    class AssetRegister
    {
    public:
        void Init();
        void Shutdown();
        std::string GetInfo(UUID key);

        template<typename T, typename I>
        UUID Register(const I& info, std::string_view debugName, UUID id)
        {
            auto* manager = ServiceManager::GetService<AssetManager>();

            REGISTER_TYPE(AMesh, AMeshCreateInfo, MeshAssembler)
            else REGISTER_TYPE(AImage, AImageCreateInfo, ImageAssembler)
            else REGISTER_TYPE(AMaterial, AMaterialCreateInfo, MaterialAssembler)
            else REGISTER_TYPE(ASkeleton, ASkeletonCreateInfo, SkeletonAssembler)
            else REGISTER_TYPE(AClip, AClipCreateInfo, ClipAssembler)
            else REGISTER_TYPE(ASheet, ASheetCreateInfo, SheetAssembler)
            else AE_CORE_ERROR("cannot register this type");
            
            if (!debugName.empty()) m_DebugInfo[id] = std::string(debugName);
            return id;
        }
    private:
        std::unordered_map<UUID, std::string> m_DebugInfo;
        
        void MeshAssembler(UUID id, AssetManager* manager, const AMeshCreateInfo& info);
        void ImageAssembler(UUID id, AssetManager* manager, const AImageCreateInfo& info);
        void MaterialAssembler(UUID id, AssetManager* manager, const AMaterialCreateInfo& info);
        void SkeletonAssembler(UUID id, AssetManager* manager, const ASkeletonCreateInfo& info);
        void ClipAssembler(UUID id, AssetManager* manager, const AClipCreateInfo& info);
        void SheetAssembler(UUID id, AssetManager* manager, const ASheetCreateInfo& info);
    };
}