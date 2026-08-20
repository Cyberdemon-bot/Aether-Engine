#pragma once

#include <vector>
#include "Aether/Animation/RigModule.h"
#include "Aether/Container/ResourcePool.h"

// Ozz-animation includes
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/unique_ptr.h>

namespace Aether {

    class Ozz_RigModule : public RigModule
    {
    public:
        Ozz_RigModule();
        virtual ~Ozz_RigModule() override; 

        virtual Handle<RSkeleton> CreateSkeleton(const SkeletonCreateInfo& data) override;
        virtual Handle<RClip> CreateClip(const ClipCreateInfo& data, Handle<RSkeleton> skeleton) override;

        virtual Handle<SkeletonCache> CreateCache(Handle<RClip> clip) override;
        virtual void DestroyCache(Handle<SkeletonCache> cache) override;
        virtual void RepairCache(Handle<SkeletonCache> cache, Handle<RClip> clip) override;

        virtual Handle<Pose> CreatePose(Handle<RSkeleton> skeleton) override;
        virtual void DestroyPose(Handle<Pose> pose) override;

        virtual Handle<Mask> CreateMask(Handle<RSkeleton> skeleton, float* weights, size_t size) override;
        virtual void DestroyMask(Handle<Mask> mask) override;
        virtual void FillMaskSubtree(Handle<Mask> mask, Handle<RSkeleton> skeleton, std::string_view boneName, float weight) override;


        virtual int GetJointIndex(Handle<RSkeleton> skeleton, std::string_view name) const override;
        virtual std::string GetJointName(Handle<RSkeleton> skeleton, int index) const override;
        virtual bool GetIBM(Handle<RSkeleton> skeleton, int boneIndex, glm::mat4& out) const override;
        virtual void GetRestPoseMatrices(Handle<RSkeleton> skeleton, glm::mat4* arr, size_t size) const override;
        virtual PoseData GetPose(Handle<Pose> pose) override;

        virtual void ScheduleSample(  
            Handle<RSkeleton> skeleton,
            Handle<RClip> clip, 
            Handle<SkeletonCache> cache, 
            Handle<Pose> poseOut,
            float time) override;

        virtual void ScheduleBlend(
            Handle<Pose> poseA,
            Handle<Pose> poseB,
            Handle<Pose> poseOut,
            float alpha) override;
        
        virtual void ScheduleAdditive(
            Handle<Pose> poseBase,
            Handle<Pose> poseAdditive,
            Handle<Pose> poseOut,
            float weight) override;

        virtual void ScheduleLayeredBlend(
            Handle<Pose> poseA,
            Handle<Pose> poseB,
            Handle<Mask> mask,
            Handle<Pose> poseOut) override;

        virtual void ScheduleTwoBoneIK(const TwoBoneIKSpec& spec) override;
        virtual void ScheduleLookAt(const LookAtSpec& spec) override;

        virtual void ScheduleFinalize(
            Handle<RSkeleton> skeleton,
            Handle<Pose> pose) override;

        virtual void ProcessTasks() override;
        virtual void ClearTasks() override;
    private:
        struct OzzSkeleton
        {
            ozz::unique_ptr<ozz::animation::Skeleton> data;
            std::vector<glm::mat4> orderedIBMs;

            OzzSkeleton() = default;
            OzzSkeleton(const OzzSkeleton&) = delete;
            OzzSkeleton& operator=(const OzzSkeleton&) = delete;
            OzzSkeleton(OzzSkeleton&&) = default;
            OzzSkeleton& operator=(OzzSkeleton&&) = default;
        };

        struct OzzClip
        {
            ozz::unique_ptr<ozz::animation::Animation> data;

            OzzClip() = default;
            OzzClip(const OzzClip&) = delete;
            OzzClip& operator=(const OzzClip&) = delete;
            OzzClip(OzzClip&&) = default;
            OzzClip& operator=(OzzClip&&) = default;
        };

        struct OzzCache
        {
            ozz::unique_ptr<ozz::animation::SamplingJob::Context> data;
        };

        struct OzzPose
        {
            ozz::vector<ozz::math::SoaTransform> localTransforms; 
            std::vector<glm::mat4> finalMats;
        };
        

        struct OzzMask
        {
            std::vector<float> weights;
        };

        struct SampleTask
        {
            Handle<RSkeleton> skeleton;
            Handle<RClip> clip;
            Handle<SkeletonCache> cache;
            float time;
            Handle<Pose> poseOut;
        };

        enum class BlendMode { Lerp, Additive, Layered };
        struct BlendTask
        {
            BlendMode mode;
            Handle<Pose> poseA;
            Handle<Pose> poseB;
            Handle<Mask> mask;   
            float alpha; 
            Handle<Pose> poseOut;
        };

        enum class IKMode { TwoBone, LookAt };

        struct IKTask
        {
            IKMode mode;
            TwoBoneIKSpec TBspec; 
            LookAtSpec LAspec; 
        };

        struct FinalizeTask
        {
            Handle<RSkeleton> skeleton;
            Handle<Pose> pose;
        };
        
        ozz::unique_ptr<ozz::animation::Skeleton> ConvertToOzzSkeleton(const SkeletonCreateInfo& data);
        ozz::unique_ptr<ozz::animation::Animation> ConvertToOzzAnimation(const ClipCreateInfo& data, int numJoints);
        void ConvertOzzMatrixToGlm(const ozz::math::Float4x4& ozzMat, glm::mat4& glmMat) const;
        void BuildHierarchy(const SkeletonCreateInfo& data, int parentIdx, ozz::animation::offline::RawSkeleton::Joint& outJoint);

        void ExecuteSampleTasks();
        void ExecuteBlendTasks();
        void ExecuteIKTasks();
        void ExecuteFinalizeTasks();

        void SampleClipIntoPose(const SampleTask& task);
        void BlendPoses(const BlendTask& task);
        void ApplyTwoBoneIK(const IKTask& task);
        void ApplyLookAt(const IKTask& task);
        void FinalizePose(const FinalizeTask& task);
    private:
        ResourcePool<Handle<RSkeleton>, OzzSkeleton> m_SkeletonPool;
        ResourcePool<Handle<RClip>, OzzClip> m_ClipPool;
        ResourcePool<Handle<SkeletonCache>, OzzCache> m_CachePool;
        ResourcePool<Handle<Pose>, OzzPose> m_PosePool;
        ResourcePool<Handle<Mask>, OzzMask> m_MaskPool;

        std::vector<SampleTask> m_SampleTasks;
        std::vector<BlendTask> m_BlendTasks;
        std::vector<IKTask> m_IKTasks;
        std::vector<FinalizeTask> m_FinalizeTasks;
    };

}