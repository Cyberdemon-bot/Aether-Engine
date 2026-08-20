#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include "Aether/Animation/RigModule.h"

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Aether {

    struct LSkeletonCreateInfo
    {
        UUID AssetID;
        std::string DebugName;
        SkeletonCreateInfo spec;
    };

    struct LClipCreateInfo
    {
        UUID AssetID;
        std::string DebugName;
        uint32_t rigIdx;
        ClipCreateInfo spec;
    };

    struct RigAnimsCreateInfo
    {
        std::vector<LSkeletonCreateInfo> rigs;
        std::vector<LClipCreateInfo> clips;
    };

    class AnimationParser
    {
    public:
        virtual ~AnimationParser() = default;
        virtual Ref<RigAnimsCreateInfo> ParseRigAnim(void* data) = 0;
        static Ref<AnimationParser> Create();
    };
}