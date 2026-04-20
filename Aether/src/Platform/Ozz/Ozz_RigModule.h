#pragma once

#include "Aether/Animation/RigModule.h"
#include "Aether/Container/ResourcePool.h"
#include <vector>

// Ozz-animation includes
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/unique_ptr.h>
#include <ozz/animation/offline/raw_skeleton.h>

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

        virtual Handle<TaskTag> CalcPose(Handle<SkeletonTag> skeleton, Handle<ClipTag> clip, Handle<CacheTag> cache, float time) override;
        virtual std::tuple<const glm::mat4*, size_t> GetPose(Handle<TaskTag> handle) const override;
        virtual int GetBoneIndex(Handle<SkeletonTag> skeleton, const std::string& name) const override;
        virtual float GetDuration(Handle<ClipTag> clip) const override;
        virtual int GetJointCount(Handle<SkeletonTag> skeleton) const override;
        virtual void GetRestPoseMatrices(Handle<SkeletonTag> skeleton, glm::mat4* arr, size_t size) const override;
        virtual bool GetIBM(Handle<SkeletonTag> skeleton, int boneIndex, glm::mat4& out) const override;
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

        void CalculateMatrices(glm::mat4* out, glm::mat4* ibm, size_t size, ozz::animation::Skeleton* skeleton, ozz::animation::Animation* clip, ozz::animation::SamplingJob::Context* cache, float time);
        ozz::unique_ptr<ozz::animation::Skeleton> ConvertToOzzSkeleton(const SkeletonSpec& data);
        ozz::unique_ptr<ozz::animation::Animation> ConvertToOzzAnimation(const ClipSpec& data, int numJoints);
        void ConvertOzzMatrixToGlm(const ozz::math::Float4x4& ozzMat, glm::mat4& glmMat) const;
        void BuildHierarchy(const SkeletonSpec& data, int parentIdx, ozz::animation::offline::RawSkeleton::Joint& outJoint);
    private:
        ResourcePool<Handle<SkeletonTag>, OzzSkeleton> m_SkeletonPool;
        ResourcePool<Handle<ClipTag>, OzzClip> m_ClipPool;
        ResourcePool<Handle<CacheTag>, OzzCache> m_CachePool;
        ResourcePool<Handle<TaskTag>, std::tuple< Handle<SkeletonTag>, Handle<ClipTag>, Handle<CacheTag>, float, std::vector<glm::mat4> >> m_TaskPool;
    };

}