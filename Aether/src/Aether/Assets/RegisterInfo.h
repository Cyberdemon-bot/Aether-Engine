#pragma once

#include <tuple>
#include <glm/glm.hpp>
#include "Aether/Core/Delegate.h"
#include "Aether/Renderer/Texture.h"
#include "Aether/Animation/RigModule.h"

namespace Aether {

    struct AMeshCreateInfo;
    struct VertexStream;
    using BoundsCalculator = Delegate<std::tuple<glm::vec3, glm::vec3>(const AMeshCreateInfo&)>;
    using AnimatedBoundsCalculator = Delegate<std::tuple<glm::vec3, glm::vec3>(const AMeshCreateInfo&)>;

    struct AMeshCreateInfo
    {
        uint32_t streamLen = 0, indexLen = 0;
        const VertexStream* streams = nullptr;
        const uint32_t* indicies = nullptr;
        std::vector<SubMesh> submeshes = {};
        std::vector<glm::mat4> poseMats = {};
        BoundsCalculator CalculateBoundsFunc;
        AnimatedBoundsCalculator CalculateAnimatedBoundsFunc;
    };


    struct AImageCreateInfo
    {
        TextureCreateInfo layout;
        std::vector<uint8_t> raw;
    };

    struct AMaterialCreateInfo
    {
        glm::vec4 albedo = {1.0f, 1.0f, 1.0f, 1.0f};
        float metallic = 0.0f;
        float roughness = 1.0f;
        int albedoMapIdx = -1;
        int normalMapIdx = -1;
        int metallicRoughnessMapIdx = -1;

        UUID* imageList = nullptr; uint32_t imageSize = 0;
    };

    using ASkeletonCreateInfo = SkeletonCreateInfo;

    struct AClipCreateInfo
    {
        ClipCreateInfo layout;
        UUID skeleton;
    };

    struct ASheetCreateInfo
    {
        UUID* matList; uint32_t matSize;
    };
}