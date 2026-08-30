#include "aepch.h"
#include "Aether/Importer/Importer.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/FileSystem/FileSystem.h"
#include "Aether/Assets/AssetRegister.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Core/Assert.h"

namespace Aether {

    

    void Importer::Init()
    {
        // GLTFConverter is stateless (no API selection, nothing to spin up) -
        // kept as a no-op only to preserve the existing Init/Shutdown call
        // sites elsewhere in the engine's startup sequence.
    }

    void Importer::Shutdown()
    {
    }

    ParsedScene Importer::ImportScene(const std::string& path)
    {
        auto* fs = ServiceManager::GetService<FileSystem>();
        fs->RegisterPath(path);
        fs->CommitRegistry();
        auto handle = fs->Open(path);
        Ref<SceneHierarchy> scene;
        Ref<CreateInfoList> result = m_Converter.Import(fs->GetBytes(handle), scene);
        fs->Close(handle);
        return ParsedScene{scene, result};
    }

    std::string Importer::ImportText(const std::string& path)
    {
        auto* fs = ServiceManager::GetService<FileSystem>();
        fs->RegisterPath(path);
        fs->CommitRegistry();
        auto handle = fs->Open(path);
        auto temp = fs->GetBytes(handle);
        std::string res(reinterpret_cast<const char*>(temp.bytes), temp.size);
        fs->Close(handle);
        return res;
    }

    RegisteredScene Importer::UploadScene(const ParsedScene& sceneData)
    {
        RegisteredScene res;

        auto* assetRegister = ServiceManager::GetService<AssetRegister>();
        auto* assetManager = ServiceManager::GetService<AssetManager>();
        auto  animSystem = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();

        sceneData.result->ForEach<AImageCreateInfo>(AssetType::Image, [&](const AImageCreateInfo& img)
        {
            assetRegister->Register<AImage>(img);
        });

        sceneData.result->ForEach<AMaterialCreateInfo>(AssetType::Material, [&](const AMaterialCreateInfo& mat)
        {
            assetRegister->Register<AMaterial>(mat);
            res.matIDs.push_back(mat.id);
        });

        sceneData.result->ForEach<ASheetCreateInfo>(AssetType::Sheet, [&](const ASheetCreateInfo& sheet)
        {
            assetRegister->Register<ASheet>(sheet);
            res.sheetIDs.push_back(sheet.id);
        });

        sceneData.result->ForEach<ASkeletonCreateInfo>(AssetType::Skeleton, [&](const ASkeletonCreateInfo& skel)
        {
            assetRegister->Register<ASkeleton>(skel);
            res.animators.push_back({});
            res.animators.back().skeleton = skel.id;
        });

        sceneData.result->ForEach<AClipCreateInfo>(AssetType::Clip, [&](const AClipCreateInfo& clip)
        {
            assetRegister->Register<AClip>(clip);
            for (auto& animator : res.animators)
            {
                if (animator.skeleton == clip.skeleton)
                {
                    animator.clips.push_back(clip.id);
                    break;
                }
            }
        });

        sceneData.result->ForEach<AMeshCreateInfo>(AssetType::Mesh, [&](const AMeshCreateInfo& meshInfo)
        {
            assetRegister->Register<AMesh>(meshInfo);
            res.meshIDs.push_back(meshInfo.id);
        });

        res.hierarchy = sceneData.scene;
        return res;
    }
}