#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/Timestep.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Container/ResourcePool.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp> 
#include <tuple>

namespace Aether {

    struct TaskTag;
    struct CacheTag;
    struct ClipTag;
    struct SkeletonTag;

    struct SkeletonSpec
    {
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
    };

    struct ClipSpec
    {
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

    class RigModule : public AnimationModule
    {
    public:
        virtual Handle<SkeletonTag> CreateSkeleton(const SkeletonSpec& data) = 0;
        virtual Handle<ClipTag> CreateClip(const ClipSpec& data, Handle<SkeletonTag> skeleton) = 0;
        virtual Handle<CacheTag> CreateCache(Handle<ClipTag> clip) = 0;
        virtual void DestroyCache(Handle<CacheTag> cache) = 0;

        virtual Handle<TaskTag> CalcPose(Handle<SkeletonTag> skeleton, Handle<ClipTag> clip, Handle<CacheTag> cache, float time) = 0;
        virtual std::tuple<const glm::mat4*, size_t> GetPose(Handle<TaskTag> handle) const = 0;
        virtual int GetBoneIndex(Handle<SkeletonTag> skeleton, const std::string& name) const = 0;
        virtual float GetDuration(Handle<ClipTag> clip) const = 0;
        virtual int GetJointCount(Handle<SkeletonTag> skeleton) const = 0;
        virtual void GetRestPoseMatrices(Handle<SkeletonTag> skeleton, glm::mat4* arr, size_t size) const = 0;
        virtual bool GetIBM(Handle<SkeletonTag> skeleton, int boneIndex, glm::mat4& out) const = 0;
        virtual void ProcessTasks() = 0;
        virtual void ClearTasks() = 0;
    private:
        static Ref<RigModule> Create();
        static const char* ModuleName() { return "RigModule"; }
        const char* GetName() const override { return "RigModule"; }
        friend class AnimationSystem;
    };

}