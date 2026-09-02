#pragma once
#include <string> 
#include <type_traits>
#include <unordered_map>
#include "Aether/Core/UUID.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Assets/AssetCreateInfo.h"
#include "Aether/Assets/CreateInfoList.h"
#include "Aether/Assets/BatchRegisterResult.h"

#define REGISTER_TYPE(Type, InfoType, AssemblerFunc) \
    if constexpr (std::is_same_v<T, Type> && std::is_same_v<I, InfoType>) \
        AssemblerFunc(manager, info);

namespace Aether {
    class AssetRegister
    {
    public:
        void Init();
        void Shutdown();
        std::string GetInfo(UUID key);

        

        template<typename T, typename I>
        UUID Register(const I& info)
        {
            auto* manager = ServiceManager::GetService<AssetManager>();

            REGISTER_TYPE(AMesh, AMeshCreateInfo, MeshAssembler)
            else REGISTER_TYPE(AImage, AImageCreateInfo, ImageAssembler)
            else REGISTER_TYPE(AMaterial, AMaterialCreateInfo, MaterialAssembler)
            else REGISTER_TYPE(ASkeleton, ASkeletonCreateInfo, SkeletonAssembler)
            else REGISTER_TYPE(AClip, AClipCreateInfo, ClipAssembler)
            else REGISTER_TYPE(ASheet, ASheetCreateInfo, SheetAssembler)
            else REGISTER_TYPE(AAudio, AAudioCreateInfo, AudioAssambler)
            else AE_CORE_ERROR("cannot register this type");
            
            if (!info.debugName.empty()) m_DebugInfo[info.id] = std::move(info.debugName);
            return info.id;
        }

        BatchRegisterResult RegisterBatch(Ref<CreateInfoList> createInfoList);
    private:
        std::unordered_map<UUID, std::string> m_DebugInfo;
        
        void MeshAssembler(AssetManager* manager, const AMeshCreateInfo& info);
        void ImageAssembler(AssetManager* manager, const AImageCreateInfo& info);
        void MaterialAssembler(AssetManager* manager, const AMaterialCreateInfo& info);
        void SkeletonAssembler(AssetManager* manager, const ASkeletonCreateInfo& info);
        void ClipAssembler(AssetManager* manager, const AClipCreateInfo& info);
        void SheetAssembler(AssetManager* manager, const ASheetCreateInfo& info);
        void AudioAssambler(AssetManager* manager, const AAudioCreateInfo& info);

       template<typename TAsset, typename TCreateInfo>
        void ProcessAssetGroup(AssetType type, const CreateInfoList* list, BatchRegisterResult& outResult)
        {
            uint32_t startOffset = static_cast<uint32_t>(outResult.m_AllIDs.size());
            uint32_t count = 0;

            list->ForEach<TCreateInfo>(type, [&](const TCreateInfo& info)
            {
                Register<TAsset>(info);
                outResult.m_AllIDs.push_back(info.id);
                count++;
            });
            outResult.m_Slices[static_cast<size_t>(type)] = { startOffset, count };
        }
    };
}