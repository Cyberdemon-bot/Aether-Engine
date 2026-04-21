#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <glm/glm.hpp>

namespace Aether
{
    struct TextureSpec;
    struct TextureCreateInfo;
    struct MaterialCreateInfo;

    struct MatHeader
    {
        uint32_t Magic = 'MATF';
        uint32_t Version = 1;

        uint32_t TextureCount = 0;
        uint32_t MaterialCount = 0;

        uint64_t TextureOffset = 0;
        uint64_t MaterialOffset = 0;
    };

    bool WriteMatFile(
        const std::string& path,
        const std::vector<TextureCreateInfo>& textures,
        const std::vector<MaterialCreateInfo>& materials
    );

    bool ReadMatFile(
        const std::string& path,
        std::vector<TextureCreateInfo>& outTextures,
        std::vector<MaterialCreateInfo>& outMaterials
    );
} 