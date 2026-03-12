#pragma once

#include "Aether/Core/Base.h"
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
        
        struct Joint
        {
            std::string Name;
            int16_t ParentIndex;
            glm::vec3 Translation;
            glm::quat Rotation;
            glm::vec3 Scale;
        };
        
        std::vector<Joint> Joints;
        std::vector<glm::mat4> IBM;
        glm::mat4 nodeTransform;
    };

    struct ClipCreateInfo
    {
        std::string DebugName;
        float Duration;
        float SampleRate;  
        
        struct Track
        {
            int JointIndex;  
            
            std::vector<float> TranslationTimes;
            std::vector<glm::vec3> TranslationValues;
            
            std::vector<float> RotationTimes;
            std::vector<glm::quat> RotationValues;
            
            std::vector<float> ScaleTimes;
            std::vector<glm::vec3> ScaleValues;
        };
        
        std::vector<Track> Tracks;
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