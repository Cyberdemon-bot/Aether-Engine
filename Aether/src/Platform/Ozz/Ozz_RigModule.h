#pragma once

#include "Aether/Animation/RigModule.h"
#include "Aether/Container/ResourcePool.h"
#include <vector>

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

        virtual Handle<SkeletonTag> CreateSkeleton(const SkeletonSpec& data) override;
        virtual Handle<ClipTag> CreateClip(const ClipSpec& data, Handle<SkeletonTag> skeleton) override;

        virtual Handle<CacheTag> CreateCache(Handle<ClipTag> clip) override;
        virtual void DestroyCache(Handle<CacheTag> cache) override;
        virtual void RepairCache(Handle<CacheTag> cache, Handle<ClipTag> clip) override;

        virtual Handle<PoseTag> CreatePose(Handle<SkeletonTag> skeleton) override;
        virtual void DestroyPose(Handle<PoseTag> pose) override;

        virtual Handle<MaskTag> CreateMask(Handle<SkeletonTag> skeleton, float* weights, size_t size) override;
        virtual void DestroyMask(Handle<MaskTag> mask) override;
        virtual void FillMaskSubtree(Handle<MaskTag> mask, Handle<SkeletonTag> skeleton, const std::string& boneName, float weight) override;


        virtual int GetJointIndex(Handle<SkeletonTag> skeleton, const std::string& name) const override;
        virtual std::string GetJointName(Handle<SkeletonTag> skeleton, int index) const override;
        virtual float GetDuration(Handle<ClipTag> clip) const override;
        virtual int GetJointCount(Handle<SkeletonTag> skeleton) const override;
        virtual bool GetIBM(Handle<SkeletonTag> skeleton, int boneIndex, glm::mat4& out) const override;
        virtual void GetRestPoseMatrices(Handle<SkeletonTag> skeleton, glm::mat4* arr, size_t size) const override;
        virtual std::tuple<const glm::mat4*, size_t> GetPose(Handle<PoseTag> pose) override;

        virtual void ScheduleSample(  
            Handle<SkeletonTag> skeleton,
            Handle<ClipTag> clip, 
            Handle<CacheTag> cache, 
            Handle<PoseTag> poseOut,
            float time) override;

        virtual void ScheduleBlend(
            Handle<PoseTag> poseA,
            Handle<PoseTag> poseB,
            Handle<PoseTag> poseOut,
            float alpha) override;
        
        virtual void ScheduleAdditive(
            Handle<PoseTag> poseBase,
            Handle<PoseTag> poseAdditive,
            Handle<PoseTag> poseOut,
            float weight) override;

        virtual void ScheduleLayeredBlend(
            Handle<PoseTag> poseA,
            Handle<PoseTag> poseB,
            Handle<MaskTag> mask,
            Handle<PoseTag> poseOut) override;

        virtual void ScheduleTwoBoneIK(const TwoBoneIKSpec& spec) override;
        virtual void ScheduleLookAt(const LookAtSpec& spec) override;

        virtual void ScheduleFinalize(
            Handle<SkeletonTag> skeleton,
            Handle<PoseTag> pose) override;

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
            Handle<SkeletonTag> skeleton;
            Handle<ClipTag> clip;
            Handle<CacheTag> cache;
            float time;
            Handle<PoseTag> poseOut;
        };

        enum class BlendMode { Lerp, Additive, Layered };
        struct BlendTask
        {
            BlendMode mode;
            Handle<PoseTag> poseA;
            Handle<PoseTag> poseB;
            Handle<MaskTag> mask;   
            float alpha; 
            Handle<PoseTag> poseOut;
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
            Handle<SkeletonTag> skeleton;
            Handle<PoseTag> pose;
        };
        
        ozz::unique_ptr<ozz::animation::Skeleton> ConvertToOzzSkeleton(const SkeletonSpec& data);
        ozz::unique_ptr<ozz::animation::Animation> ConvertToOzzAnimation(const ClipSpec& data, int numJoints);
        void ConvertOzzMatrixToGlm(const ozz::math::Float4x4& ozzMat, glm::mat4& glmMat) const;
        void BuildHierarchy(const SkeletonSpec& data, int parentIdx, ozz::animation::offline::RawSkeleton::Joint& outJoint);

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
        ResourcePool<Handle<SkeletonTag>, OzzSkeleton> m_SkeletonPool;
        ResourcePool<Handle<ClipTag>, OzzClip> m_ClipPool;
        ResourcePool<Handle<CacheTag>, OzzCache> m_CachePool;
        ResourcePool<Handle<PoseTag>, OzzPose> m_PosePool;
        ResourcePool<Handle<MaskTag>, OzzMask> m_MaskPool;

        std::vector<SampleTask> m_SampleTasks;
        std::vector<BlendTask> m_BlendTasks;
        std::vector<IKTask> m_IKTasks;
        std::vector<FinalizeTask> m_FinalizeTasks;
    };

}