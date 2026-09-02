#pragma once

#include <tuple>
#include <string>
#include <span>
#include <glm/glm.hpp>
#include "Aether/Core/UUID.h"
#include "Aether/Core/Delegate.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Renderer/Texture.h"
#include "Aether/Animation/RigModule.h"

namespace Aether {

    struct AssetCreateInfo
    {
        UUID id;
        std::string debugName;
    };

    struct AMeshCreateInfo : public AssetCreateInfo
    {
        std::span<const VertexStream> streams;
        std::span<const uint32_t> indicies;
        std::span<const Submesh> Submeshes;
        glm::vec3 boundsMin = glm::vec3(0.0f), boundsMax = glm::vec3(0.0f),
        animatedBoundsMin = glm::vec3(0.0f), animatedBoundsMax = glm::vec3(0.0f);
        bool hasJointData = false;
    };


    struct AImageCreateInfo : public AssetCreateInfo
    {
        TextureCreateInfo layout;
        std::span<const uint8_t> raw;
    };

    struct AMaterialCreateInfo : public AssetCreateInfo
    {
        glm::vec4 albedo = {1.0f, 1.0f, 1.0f, 1.0f};
        UUID albedoMap, normalMap, metallicRoughnessMap;
        float metallic = 0.0f;
        float roughness = 1.0f;
    };

    struct ASkeletonCreateInfo : public AssetCreateInfo
    {
        SkeletonCreateInfo data;
    };

    struct AClipCreateInfo : public AssetCreateInfo
    {
        ClipCreateInfo data;
        UUID skeleton;
    };

    struct ASheetCreateInfo : public AssetCreateInfo
    {
        std::span<const UUID> materialList;
    };
}