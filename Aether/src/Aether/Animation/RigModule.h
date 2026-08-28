#pragma once

#include <span>
#include <string>
#include <string_view>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp> 
#include "Aether/Core/Base.h"
#include "Aether/Core/Timestep.h"
#include "Aether/Container/ResourcePool.h"

namespace Aether {

    struct Clip;
    struct Skeleton;
    struct Pose;
    struct Mask;
    struct SkeletonCache;

    struct SkeletonCreateInfo
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

    struct ClipCreateInfo
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

    struct TwoBoneIKSpec
    {
        Handle<Skeleton> Skeleton;
        Handle<Pose> Pose;
        int Root, Mid, End;
        glm::vec3 Target, Pole;
        float Weight;
    };

    struct LookAtSpec
    {
        Handle<Skeleton> Skeleton;
        Handle<Pose> Pose;
        int Bone;
        glm::vec3 Target, Forward, Up;
        float Weight, AngleLimit; //rad
    };

    class RigModule
    {
    public:
        virtual ~RigModule() = default;
        
        virtual Handle<Skeleton> CreateSkeleton(const SkeletonCreateInfo& data) = 0;
        virtual Handle<Clip> CreateClip(const ClipCreateInfo& data, Handle<Skeleton> skeleton) = 0;

        virtual Handle<SkeletonCache> CreateCache(Handle<Clip> clip) = 0;
        virtual void DestroyCache(Handle<SkeletonCache> cache) = 0;
        virtual void RepairCache(Handle<SkeletonCache> cache, Handle<Clip> clip) = 0;

        virtual Handle<Pose> CreatePose(Handle<Skeleton> skeleton) = 0;
        virtual void DestroyPose(Handle<Pose> pose) = 0;

        virtual Handle<Mask> CreateMask(Handle<Skeleton> skeleton, float* weights, size_t size) = 0;
        virtual void DestroyMask(Handle<Mask> mask) = 0;
        virtual void FillMaskSubtree(Handle<Mask> mask, Handle<Skeleton> skeleton, std::string_view boneName, float weight) = 0;

        virtual int GetJointIndex(Handle<Skeleton> skeleton, std::string_view name) const = 0;
        virtual std::string GetJointName(Handle<Skeleton> skeleton, int index) const = 0;
        virtual bool GetIBM(Handle<Skeleton> skeleton, int boneIndex, glm::mat4& out) const = 0;
        virtual std::span<const glm::mat4> GetPose(Handle<Pose> pose) = 0;

        virtual void ScheduleSample(  
            Handle<Skeleton> skeleton,
            Handle<Clip> clip, 
            Handle<SkeletonCache> cache, 
            Handle<Pose> poseOut,
            float time) = 0;

        virtual void ScheduleBlend(
            Handle<Pose> poseA,
            Handle<Pose> poseB,
            Handle<Pose> poseOut,
            float alpha) = 0;
        
        virtual void ScheduleAdditive(
            Handle<Pose> poseBase,
            Handle<Pose> poseAdditive,
            Handle<Pose> poseOut,
            float weight) = 0;

        virtual void ScheduleLayeredBlend(
            Handle<Pose> poseA,
            Handle<Pose> poseB,
            Handle<Mask> mask,
            Handle<Pose> poseOut) = 0;

        virtual void ScheduleTwoBoneIK(const TwoBoneIKSpec& spec) = 0;
        virtual void ScheduleLookAt(const LookAtSpec& spec) = 0;

        virtual void ScheduleFinalize(
            Handle<Skeleton> skeleton,
            Handle<Pose> pose) = 0;

        virtual void ProcessTasks() = 0;
        virtual void ClearTasks() = 0;
    private:
        static Scope<RigModule> Create();
        friend class AnimationSystem;
    };

}