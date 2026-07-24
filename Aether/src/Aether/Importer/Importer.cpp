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

    Ref<ParsedScene> Importer::Import(const std::string& path, bool registerPath) 
    {
        auto it = ServiceManager::GetService<FileSystem>();
        if (registerPath)
        {
            it->RegisterPath(path);
            it->CommitRegistry();
        }
        auto handle = it->Open(path);
        auto res = s_GLBAssembler->Import(it->GetBytes(handle));
        it->Close(handle);
        return res;
    }

    RegisteredScene Importer::Upload(const Ref<ParsedScene>& sceneData) 
    {
        return s_GLBAssembler->Upload(sceneData);
    }
}