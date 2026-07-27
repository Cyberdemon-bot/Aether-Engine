#include "Aether/Core/ServiceManager.h"
#include "Aether/FileSystem/FileSystem.h"
#include "Aether/Importer/Importer.h"

namespace Aether {
    void Importer::Init()
    {
        s_GLBAssembler = GLBAssembler::Create();
    }

    void Importer::Shutdown()
    {
        s_GLBAssembler.reset();
    }

    Ref<ParsedScene> Importer::ImportScene(const std::string& path) 
    {
        auto it = ServiceManager::GetService<FileSystem>();
        it->RegisterPath(path);
        it->CommitRegistry();
        auto handle = it->Open(path);
        auto res = s_GLBAssembler->Import(it->GetBytes(handle));
        it->Close(handle);
        return res;
    }

    std::string Importer::ImportText(const std::string& path)
    {
        auto it = ServiceManager::GetService<FileSystem>();
        it->RegisterPath(path);
        it->CommitRegistry();
        auto handle = it->Open(path);
        auto temp = it->GetBytes(handle);
        std::string res(reinterpret_cast<const char*>(temp.bytes), temp.size);
        it->Close(handle);
        return res;
    }

    RegisteredScene Importer::UploadScene(const Ref<ParsedScene>& sceneData) 
    {
        return s_GLBAssembler->Upload(sceneData);
    }
}