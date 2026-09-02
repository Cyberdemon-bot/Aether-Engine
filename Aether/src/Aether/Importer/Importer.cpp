#include "aepch.h"
#include "Aether/Importer/Importer.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/FileSystem/FileSystem.h"
#include "Aether/Assets/AssetRegister.h"
#include "Aether/Core/Assert.h"

namespace Aether {

    void Importer::Init()
    {
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

    UUID Importer::ImportAudio(const std::string& path)
    {
        auto fs = ServiceManager::GetService<FileSystem>();
        auto* assetRegister = ServiceManager::GetService<AssetRegister>();
        fs->RegisterPath(path);
        fs->CommitRegistry();

        auto handle = fs->Open(path);
        auto temp = fs->GetBytes(handle);

        AAudioCreateInfo info;
        info.id = UUID(); 
        info.raw = std::span(temp.Data(), temp.Size());
        assetRegister->Register<AAudio>(info);
        fs->Close(handle);

        return info.id;
    }

    RegisteredScene Importer::UploadScene(const ParsedScene& sceneData)
    {
        RegisteredScene res;

        auto* assetRegister = ServiceManager::GetService<AssetRegister>();
        res.assets = assetRegister->RegisterBatch(sceneData.result);
        res.hierarchy = sceneData.scene;
        return res;
    }


}