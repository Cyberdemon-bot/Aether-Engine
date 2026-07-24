#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Importer/GLBAssembler.h"

namespace Aether {
    class AETHER_API Importer
    {
    public: 
        void Init();
        void Shutdown();

        Ref<ParsedScene> Import(const std::string& path);
        RegisteredScene Upload(const Ref<ParsedScene>& sceneData);
    private:
        Scope<GLBAssembler> s_GLBAssembler;
    };
}