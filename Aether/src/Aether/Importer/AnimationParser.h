#pragma once

#include "Aether/Core/Base.h"
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
        std::string DebugName;
        SkeletonSpec spec;
    };

    struct ClipCreateInfo
    {
        std::string DebugName;
        ClipSpec spec;
    };

    struct RigAnimsCreateInfo
    {
        std::vector<RigCreateInfo> rigs;
        std::vector<ClipCreateInfo> clips;
        std::unordered_map<uint32_t, std::vector<uint32_t>> rig_map;
    };

    class AnimationParser
    {
    public:
        virtual ~AnimationParser() = default;
        virtual Ref<RigAnimsCreateInfo> ParseRigAnim(void* data) = 0;
        static Ref<AnimationParser> Create();
    };
}