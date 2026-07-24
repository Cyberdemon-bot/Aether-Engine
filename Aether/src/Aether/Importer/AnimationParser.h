#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include "Aether/Animation/RigModule.h"

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Aether {

    struct SkeletonCreateInfo
    {
        UUID AssetID;
        std::string DebugName;
        SkeletonSpec spec;
    };

    struct ClipCreateInfo
    {
        UUID AssetID;
        std::string DebugName;
        uint32_t rigIdx;
        ClipSpec spec;
    };

    struct RigAnimsCreateInfo
    {
        std::vector<SkeletonCreateInfo> rigs;
        std::vector<ClipCreateInfo> clips;
    };

    class AnimationParser
    {
    public:
        virtual ~AnimationParser() = default;
        virtual Ref<RigAnimsCreateInfo> ParseRigAnim(void* data) = 0;
        static Ref<AnimationParser> Create();
    };
}