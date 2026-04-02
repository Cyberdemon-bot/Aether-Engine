#include "Platform/Ozz/Ozz_RigModule.h"
#include "Aether/Core/JobSystem.h"
#include "Aether/Core/Log.h"

#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/base/maths/simd_math.h> 
#include <glm/gtc/type_ptr.hpp>

namespace Aether {

    Ozz_RigModule::Ozz_RigModule()
    {

    }

    Ozz_RigModule::~Ozz_RigModule()
    {
        
    }

    Handle<SkeletonTag> Ozz_RigModule::CreateSkeleton(const SkeletonSpec& data)
    {
        auto handle = m_SkeletonPool.CreateResource();
        auto* skeleton = m_SkeletonPool.GetResource(handle);
        skeleton->data = ConvertToOzzSkeleton(data);
        if (!skeleton->data || !skeleton->data->num_joints())
        {
            AE_CORE_ERROR("Failed to build ozz skeleton!");
            m_SkeletonPool.DestroyResource(handle);
            return Handle<SkeletonTag>::MakeInvalid();
        }

        const auto& names = skeleton->data->joint_names();
        int num = names.size();

        skeleton->orderedIBMs.resize(num);
        for (int i = 0; i < num; i++)
        {
            for (int j = 0; j < data.Joints.size(); j++)
            {
                if (data.Joints[j].Name == names[i])
                {
                    skeleton->orderedIBMs[i] = data.IBM[j];
                    break;
                }
            }
        }

        return handle;
    }

    int Ozz_RigModule::GetBoneIndex(Handle<SkeletonTag> skeleton, const std::string& name) const
    {
        auto it = m_SkeletonPool.GetResource(skeleton);
        if (!it) return -1;

        const auto names = it->data->joint_names();

        for (int i = 0; i < (int)names.size(); i++)
            if (names[i] == name) return i;
        return -1;
    }

    Handle<ClipTag> Ozz_RigModule::CreateClip(const ClipSpec& data, Handle<SkeletonTag> skeleton) 
    {
        auto* it = m_SkeletonPool.GetResource(skeleton);
        if (!it)
        {
            AE_CORE_ERROR("Skeleton not found for clip");
            return Handle<ClipTag>::MakeInvalid();
        }

        int num = it->data->num_joints();
        auto handle = m_ClipPool.CreateResource();
        auto* clip = m_ClipPool.GetResource(handle);
        clip->data = ConvertToOzzAnimation(data, num);

        if (!clip->data || clip->data->duration() <= 0.0f)
        {
            AE_CORE_ERROR("Failed to create ozz clip!");
            m_ClipPool.DestroyResource(handle);
            return Handle<ClipTag>::MakeInvalid();
        }

        return handle;
    }

    Handle<CacheTag> Ozz_RigModule::CreateCache(Handle<ClipTag> clip) 
    {
        auto* it = m_ClipPool.GetResource(clip);
        if (!it)
        {
            AE_CORE_ERROR("Clip not found for cache");
            return Handle<CacheTag>::MakeInvalid();
        }

        int num = it->data->num_tracks();
        auto handle = m_CachePool.CreateResource();
        auto* cache = m_CachePool.GetResource(handle);
        cache->data = ozz::make_unique<ozz::animation::SamplingJob::Context>();
        cache->data->Resize(num);
        return handle;
    }

    void Ozz_RigModule::DestroyCache(Handle<CacheTag> cache) 
    {
        auto* it = m_CachePool.GetResource(cache);
        if(!it)
        {
            AE_CORE_ERROR("Cache not found");
            return;
        }

        it->data.reset();
        m_CachePool.DestroyResource(cache);
    }

    Handle<TaskTag> Ozz_RigModule::CalcPose(Handle<SkeletonTag> skeleton, Handle<ClipTag> clip, Handle<CacheTag> cache, float time)
    {
        return m_TaskPool.CreateResource(skeleton, clip, cache, time, std::vector<glm::mat4>{});
    }

    std::tuple<const glm::mat4*, size_t> Ozz_RigModule::GetPose(Handle<TaskTag> handle) const 
    {
        const auto* it = m_TaskPool.GetResource(handle);
        if (!it) return {nullptr, 0};

        auto& [s, c, ca, time, out] = *it;
        return {out.data(), out.size()};
    }
    
    void Ozz_RigModule::ProcessTasks() 
    {
        uint32_t totalTasks = m_TaskPool.GetSize();
        if (totalTasks == 0) return;
        uint32_t chunkSize = 16; 

        JobSystem::ParallelFor(totalTasks, chunkSize, m_TaskPool.Begin(), AE_MAKE_LAMBDA((this), (auto& slot), auto, 
            auto& [s, c, ca, time, out] = slot.asset;
            auto* skeleton = m_SkeletonPool.GetResource(s);
            auto* clip = m_ClipPool.GetResource(c);
            auto* cache = m_CachePool.GetResource(ca);
            if (!skeleton || !clip || !cache) return;
            out.resize(skeleton->data->num_joints());
            CalculateMatrices(out.data(), skeleton->orderedIBMs.data(), out.size(), skeleton->data.get(), clip->data.get(), cache->data.get(), time);
        ));
    }

    void Ozz_RigModule::ClearTasks()
    {
        m_TaskPool.Clear();
    }

    float Ozz_RigModule::GetDuration(Handle<ClipTag> clip) const
    {
        auto* it = m_ClipPool.GetResource(clip);
        if (!it) return 0.0f;

        return it->data->duration();
    }

    int Ozz_RigModule::GetJointCount(Handle<SkeletonTag> skeleton) const
    {
        auto* it = m_SkeletonPool.GetResource(skeleton);
        if (!it) return 0;

        return it->data->num_joints();
    }

    void Ozz_RigModule::GetRestPoseMatrices(Handle<SkeletonTag> skeleton, glm::mat4* arr, size_t size) const
    {
        auto* it = m_SkeletonPool.GetResource(skeleton);
        if (!it) return;

        auto* skel = it->data.get(); 
        if (size < skel->num_joints()) return;
        ozz::vector<ozz::math::Float4x4> modelMats(skel->num_joints());

        ozz::animation::LocalToModelJob job;
        job.skeleton = skel;
        job.input    = skel->joint_rest_poses();
        job.output   = ozz::make_span(modelMats);
        if (!job.Run()) return;

        for (size_t i = 0; i < size; i++) 
            ConvertOzzMatrixToGlm(modelMats[i], arr[i]);
    }
    
    void Ozz_RigModule::CalculateMatrices(glm::mat4* out, glm::mat4* ibm, size_t size, ozz::animation::Skeleton* skeleton, ozz::animation::Animation* clip, ozz::animation::SamplingJob::Context* cache, float time)
    {
        if (!skeleton || !clip || !cache || size < skeleton->num_joints()) return;

        float duration = clip->duration();
        float ratio = 0.0f;
        if (duration > 0.0f)
        {
            ratio = std::fmod(time, duration) / duration;
            if (ratio < 0.0f) ratio += 1.0f;
        }

        thread_local ozz::vector<ozz::math::SoaTransform> local_transforms;
        thread_local ozz::vector<ozz::math::Float4x4> model_matrices;

        local_transforms.resize(skeleton->num_soa_joints());
        model_matrices.resize(skeleton->num_joints());

        ozz::animation::SamplingJob samplingJob;
        samplingJob.animation = clip;
        samplingJob.context = cache;
        samplingJob.ratio = ratio;
        samplingJob.output = ozz::make_span(local_transforms);

        if (!samplingJob.Run()) return;

        ozz::animation::LocalToModelJob localToModelJob;
        localToModelJob.skeleton = skeleton;
        localToModelJob.input = ozz::make_span(local_transforms);
        localToModelJob.output = ozz::make_span(model_matrices);

        if (!localToModelJob.Run()) return;

        for (int i = 0; i < size; i++)
        {
            ConvertOzzMatrixToGlm(model_matrices[i], out[i]);
            out[i] = out[i] * ibm[i];
        }
    }

    void Ozz_RigModule::BuildHierarchy(const SkeletonSpec& data, int parentIdx, ozz::animation::offline::RawSkeleton::Joint& out)
    {
        const auto& src = data.Joints[parentIdx];
        out.name = src.Name;
        out.transform.translation = ozz::math::Float3(src.Translation.x, src.Translation.y, src.Translation.z);
        out.transform.rotation = ozz::math::Quaternion(src.Rotation.x, src.Rotation.y, src.Rotation.z, src.Rotation.w);
        out.transform.scale = ozz::math::Float3(src.Scale.x, src.Scale.y, src.Scale.z);

        for (int i = 0; i < data.Joints.size(); i++)
        {
            if (data.Joints[i].ParentIndex == parentIdx)
            {
                out.children.resize(out.children.size() + 1);
                BuildHierarchy(data, i, out.children.back());
            }
        }
    }

    ozz::unique_ptr<ozz::animation::Skeleton> Ozz_RigModule::ConvertToOzzSkeleton(const SkeletonSpec& data)
    {
        using namespace ozz::animation::offline;

        RawSkeleton raw;
        int root = -1;
        for (int i = 0; i < data.Joints.size(); i++)
        {
            if (data.Joints[i].ParentIndex == -1)
            {
                root = i;
                break;
            }
        }

        if (root == -1) return nullptr;

        raw.roots.resize(1);
        BuildHierarchy(data, root, raw.roots[0]);

        if (!raw.Validate())
        {
            AE_CORE_ERROR("Skeleton validation failed!");
            return nullptr;
        }

        SkeletonBuilder builder;
        return builder(raw);
    }

    ozz::unique_ptr<ozz::animation::Animation> Ozz_RigModule::ConvertToOzzAnimation(const ClipSpec& data, int numJoints)
    {
        using namespace ozz::animation::offline;

        RawAnimation raw;
        raw.duration = data.Duration;
        raw.tracks.resize(numJoints);

        for (size_t i = 0; i < data.Tracks.size(); i++)
        {
            const auto& src = data.Tracks[i];

            int jointIdx = src.JointIndex;
            if (jointIdx < 0 || jointIdx >= numJoints) continue;

            RawAnimation::JointTrack& dst = raw.tracks[jointIdx];

            for (size_t j = 0; j < src.TranslationTimes.size(); j++)
            {
                RawAnimation::TranslationKey key;
                key.time = src.TranslationTimes[j];
                key.value = ozz::math::Float3(
                    src.TranslationValues[j].x,
                    src.TranslationValues[j].y,
                    src.TranslationValues[j].z
                );
                dst.translations.push_back(key);
            }

            for (size_t j = 0; j < src.RotationTimes.size(); j++)
            {
                RawAnimation::RotationKey key;
                key.time = src.RotationTimes[j];
                key.value = ozz::math::NormalizeSafe(ozz::math::Quaternion(
                    src.RotationValues[j].x,
                    src.RotationValues[j].y,
                    src.RotationValues[j].z,
                    src.RotationValues[j].w
                ), ozz::math::Quaternion::identity());
                dst.rotations.push_back(key);
            }

            for (size_t j = 0; j < src.ScaleTimes.size(); j++)
            {
                RawAnimation::ScaleKey key;
                key.time = src.ScaleTimes[j];
                key.value = ozz::math::Float3(
                    src.ScaleValues[j].x,
                    src.ScaleValues[j].y,
                    src.ScaleValues[j].z
                );
                dst.scales.push_back(key);
            }
        }

        AnimationBuilder builder;
        return builder(raw);
    }

    void Ozz_RigModule::ConvertOzzMatrixToGlm(const ozz::math::Float4x4& src, glm::mat4& dst) const
    {
        ozz::math::StorePtrU(src.cols[0], glm::value_ptr(dst[0]));
        ozz::math::StorePtrU(src.cols[1], glm::value_ptr(dst[1]));
        ozz::math::StorePtrU(src.cols[2], glm::value_ptr(dst[2]));
        ozz::math::StorePtrU(src.cols[3], glm::value_ptr(dst[3]));
    }
}