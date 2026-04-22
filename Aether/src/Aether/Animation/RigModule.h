#pragma once

#include <tuple>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp> 
#include "Aether/Core/Base.h"
#include "Aether/Core/Timestep.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Container/ResourcePool.h"

namespace Aether {

    struct CacheTag;
    struct ClipTag;
    struct SkeletonTag;
    struct PoseTag;
    struct MaskTag;

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

    struct TwoBoneIKSpec
    {
        Handle<SkeletonTag> Skeleton;
        Handle<PoseTag> Pose;
        int Root, Mid, End;
        glm::vec3 Target, Pole;
        float Weight;
    };

    struct LookAtSpec
    {
        Handle<SkeletonTag> Skeleton;
        Handle<PoseTag> Pose;
        int Bone;
        glm::vec3 Target, Forward, Up;
        float Weight, AngleLimit; //rad
    };

    class RigModule : public AnimationModule
    {
    public:
        virtual Handle<SkeletonTag> CreateSkeleton(const SkeletonSpec& data) = 0;
        virtual Handle<ClipTag> CreateClip(const ClipSpec& data, Handle<SkeletonTag> skeleton) = 0;

        virtual Handle<CacheTag> CreateCache(Handle<ClipTag> clip) = 0;
        virtual void DestroyCache(Handle<CacheTag> cache) = 0;

        virtual Handle<PoseTag> CreatePose(Handle<SkeletonTag> skeleton) = 0;
        virtual void DestroyPose(Handle<PoseTag> pose) = 0;

        virtual Handle<MaskTag> CreateMask(Handle<SkeletonTag> skeleton, float* weights, size_t size) = 0;
        virtual void DestroyMask(Handle<MaskTag> mask) = 0;
        virtual void FillMaskSubtree(Handle<MaskTag> mask, Handle<SkeletonTag> skeleton, const std::string& boneName, float weight) = 0;

        virtual int GetJointIndex(Handle<SkeletonTag> skeleton, const std::string& name) const = 0;
        virtual std::string GetJointName(Handle<SkeletonTag> skeleton, int index) const = 0;
        virtual float GetDuration(Handle<ClipTag> clip) const = 0;
        virtual int GetJointCount(Handle<SkeletonTag> skeleton) const = 0;
        virtual bool GetIBM(Handle<SkeletonTag> skeleton, int boneIndex, glm::mat4& out) const = 0;
        virtual void GetRestPoseMatrices(Handle<SkeletonTag> skeleton, glm::mat4* arr, size_t size) const = 0;
        virtual std::tuple<const glm::mat4*, size_t> GetPose(Handle<PoseTag> pose) = 0;

        virtual void ScheduleSample(  
            Handle<SkeletonTag> skeleton,
            Handle<ClipTag> clip, 
            Handle<CacheTag> cache, 
            Handle<PoseTag> poseOut,
            float time) = 0;

        virtual void ScheduleBlend(
            Handle<PoseTag> poseA,
            Handle<PoseTag> poseB,
            Handle<PoseTag> poseOut,
            float alpha) = 0;
        
        virtual void ScheduleAdditive(
            Handle<PoseTag> poseBase,
            Handle<PoseTag> poseAdditive,
            Handle<PoseTag> poseOut,
            float weight) = 0;

        virtual void ScheduleLayeredBlend(
            Handle<PoseTag> poseA,
            Handle<PoseTag> poseB,
            Handle<MaskTag> mask,
            Handle<PoseTag> poseOut) = 0;

        virtual void ScheduleTwoBoneIK(const TwoBoneIKSpec& spec) = 0;
        virtual void ScheduleLookAt(const LookAtSpec& spec) = 0;

        virtual void ScheduleFinalize(
            Handle<SkeletonTag> skeleton,
            Handle<PoseTag> pose) = 0;

        virtual void ProcessTasks() = 0;
        virtual void ClearTasks() = 0;
    private:
        static Ref<RigModule> Create();
        static const char* ModuleName() { return "RigModule"; }
        const char* GetName() const override { return "RigModule"; }
        friend class AnimationSystem;
    };

}