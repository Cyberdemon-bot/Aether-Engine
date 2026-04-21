#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Aether
{
    struct MeshCreateInfo;

    struct MeshHeader
    {
        uint32_t Magic = 'MESF';
        uint32_t Version = 1;

        uint32_t MeshCount = 0;
        uint64_t MeshOffset = 0;
    };

    bool WriteMeshFile(
        const std::string& path,
        const std::vector<MeshCreateInfo>& meshes
    );

    bool ReadMeshFile(
        const std::string& path,
        std::vector<MeshCreateInfo>& outMeshes
    );
}