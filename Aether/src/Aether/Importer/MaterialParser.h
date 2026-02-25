#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Renderer/Texture.h"
#include <vector>
#include <string>
#include <glm/glm.hpp>

namespace Aether {
    struct TextureCreateInfo
    {
        std::string DebugName;
        TextureSpec Spec;
        std::vector<uint8_t> RawData;
    };

    struct MaterialCreateInfo
    {
        std::string DebugName;
        glm::vec4 AlbedoColor = {1.0f, 1.0f, 1.0f, 1.0f};
        float Metallic = 0.0f;
        float Roughness = 1.0f;
        
        int AlbedoMapIdx = -1;
        int NormalMapIdx = -1;
        int MetallicRoughnessMapIdx = -1;
    };

    struct ParsedMaterialInfo
    {
        std::vector<MaterialCreateInfo> matsInfo;
        std::vector<TextureCreateInfo> texsInfo;
    };

    class MaterialParser
    {
    public:
        enum class API 
        {
            None = 0, Cgltf = 1
        };
    public:
        virtual ~MaterialParser() = default;
        virtual Ref<ParsedMaterialInfo> Parsing(void* data) = 0;
        static Ref<MaterialParser> Create();
    };
}