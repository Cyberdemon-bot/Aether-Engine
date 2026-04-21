#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include "Aether/Assets/Skeleton.h"
#include "Aether/Assets/Clip.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <set>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Aether {

    struct RigCreateInfo
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
        std::vector<RigCreateInfo> rigs;
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