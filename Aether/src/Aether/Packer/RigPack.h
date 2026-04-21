#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Aether
{
    struct RigCreateInfo;
    struct ClipCreateInfo;

    struct RigHeader
    {
        uint32_t Magic = 'RIGF';
        uint32_t Version = 1;

        uint32_t RigCount = 0;
        uint32_t ClipCount = 0;

        uint64_t RigOffset = 0;
        uint64_t ClipOffset = 0;
    };

    bool WriteRigFile(
        const std::string& path,
        const std::vector<RigCreateInfo>& rigs,
        const std::vector<ClipCreateInfo>& clips
    );

    bool ReadRigFile(
        const std::string& path,
        std::vector<RigCreateInfo>& rigs,
        std::vector<ClipCreateInfo>& clips
    );
}