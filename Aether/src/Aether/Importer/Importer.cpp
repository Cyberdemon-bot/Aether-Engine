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

    Ref<ParsedScene> Importer::ImportScene(const std::string& path)
    {
        auto* fs = ServiceManager::GetService<FileSystem>();
        fs->RegisterPath(path);
        fs->CommitRegistry();
        auto handle = fs->Open(path);
        ParsedScene scene = m_Converter.Import(fs->GetBytes(handle));
        fs->Close(handle);
        return CreateRef<ParsedScene>(std::move(scene));
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

    RegisteredScene Importer::UploadScene(const Ref<ParsedScene>& sceneData)
    {
        RegisteredScene res;

        auto* assetRegister = ServiceManager::GetService<AssetRegister>();
        auto* assetManager = ServiceManager::GetService<AssetManager>();
        auto  animSystem = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();

        for (const auto& img : sceneData->Images)
            assetRegister->Register<AImage>(img);

        for (const auto& mat : sceneData->Materials)
        {
            assetRegister->Register<AMaterial>(mat);
            res.matIDs.push_back(mat.id);
        }

        for (const auto& sheet : sceneData->Sheets)
        {
            assetRegister->Register<ASheet>(sheet);
            res.sheetIDs.push_back(sheet.id);
        }

        for (const auto& skel : sceneData->Skeletons)
        {
            assetRegister->Register<ASkeleton>(skel);
            res.animators.push_back({});
            res.animators.back().skeleton = skel.id;
        }

        for (const auto& clip : sceneData->Clips)
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
        }

        for (const auto& meshInfo : sceneData->Meshes)
        {
            assetRegister->Register<AMesh>(meshInfo);
            res.meshIDs.push_back(meshInfo.id);
        }

        res.hierarchy = sceneData->Hierarchy;
        return res;
    }
}