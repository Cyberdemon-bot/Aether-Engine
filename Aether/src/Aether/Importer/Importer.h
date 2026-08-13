#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Importer/LegacyAssembler.h"

namespace Aether {
    class AETHER_API Importer
    {
    public: 
        void Init();
        void Shutdown();

        Ref<ParsedScene> ImportScene(const std::string& path);
        RegisteredScene UploadScene(const Ref<ParsedScene>& sceneData);

        

        std::string ImportText(const std::string& path);
    private:
        Scope<LegacyAssembler> s_GLBAssembler;
    };
}