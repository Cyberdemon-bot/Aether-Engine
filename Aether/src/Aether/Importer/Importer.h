#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Importer/GLBAssembler.h"

namespace Aether {
    class AETHER_API Importer
    {
    public: 
        static Ref<ParsedScene> Import(const std::string& path) 
        {
            return s_GLBAssembler->Import(path);
        }

        static RegisteredScene Upload(const Ref<ParsedScene>& sceneData) 
        {
            return s_GLBAssembler->Upload(sceneData);
        }

        static GLBAssembler::API Get_GLB_API() 
        { 
            return GLBAssembler::GetAPI(); 
        }
    private:
        static Scope<GLBAssembler> s_GLBAssembler;
    };
}