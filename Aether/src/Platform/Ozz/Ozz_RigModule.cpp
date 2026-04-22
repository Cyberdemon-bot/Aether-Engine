#include "Platform/Ozz/Ozz_RigModule.h"
#include "Aether/Core/JobSystem.h"
#include "Aether/Core/Log.h"

#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <glm/gtc/type_ptr.hpp>

namespace Aether {

    Ozz_RigModule::Ozz_RigModule()
    {
        m_CachePool.Init();
        m_ClipPool.Init();
        m_SkeletonPool.Init();
    }

    Ozz_RigModule::~Ozz_RigModule()
    {
        m_CachePool.Shutdown();
        m_ClipPool.Shutdown();
        m_SkeletonPool.Shutdown();
    }

    //resource management

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


    Handle<PoseTag> Ozz_RigModule::CreatePose(Handle<SkeletonTag> skeleton)
    {
        auto* it = m_SkeletonPool.GetResource(skeleton);
        if (!it)
        {
            AE_CORE_ERROR("Skeleton not found for pose creation");
            return Handle<PoseTag>::MakeInvalid();
        }

        auto handle = m_PosePool.CreateResource();
        auto* pose = m_PosePool.GetResource(handle);

        pose->localTransforms.resize(it->data->num_soa_joints());
        return handle;
    }

    void Ozz_RigModule::DestroyPose(Handle<PoseTag> pose)
    {
        auto* it = m_PosePool.GetResource(pose);
        if (!it)
        {
            AE_CORE_ERROR("Pose not found");
            return;
        }
        it->localTransforms.clear();
        m_PosePool.DestroyResource(pose);
    }

    Handle<MaskTag> Ozz_RigModule::CreateMask(Handle<SkeletonTag> skeleton, float* weights, size_t size)
    {
        auto* it = m_SkeletonPool.GetResource(skeleton);
        if (!it)
        {
            AE_CORE_ERROR("Skeleton not found for mask");
            return Handle<MaskTag>::MakeInvalid();
        }

        int num = it->data->num_joints();
        if (size != num)
        {
            AE_CORE_ERROR("Mask weight count doesn't match skeleton joint count");
            return Handle<MaskTag>::MakeInvalid();
        }

        auto handle = m_MaskPool.CreateResource();
        auto* mask = m_MaskPool.GetResource(handle);
        mask->weights.assign(weights, weights + size);
        return handle;
    }

    void Ozz_RigModule::DestroyMask(Handle<MaskTag> mask)
    {
        auto* it = m_MaskPool.GetResource(mask);
        if (!it)
        {
            AE_CORE_ERROR("Mask not found");
            return;
        }
        it->weights.clear();
        m_MaskPool.DestroyResource(mask);
    }

    void Ozz_RigModule::FillMaskSubtree(Handle<MaskTag> mask, Handle<SkeletonTag> skeleton, const std::string& boneName, float weight)
    {
        auto* it  = m_MaskPool.GetResource(mask);
        auto* sk  = m_SkeletonPool.GetResource(skeleton);
        if (!it || !sk) return;

        int root = GetJointIndex(skeleton, boneName);
        if (root == -1)
        {
            AE_CORE_ERROR("FillMaskSubtree: bone '{}' not found", boneName);
            return;
        }

        const auto parents = sk->data->joint_parents();
        it->weights[root] = weight;

        const auto names = sk->data->joint_names();
        for (int i = root + 1; i < (int)names.size(); i++)
        {
            int p = parents[i];
            while (p > root) p = parents[p];
            if (p == root) it->weights[i] = weight;
        }
    }

    // task handling



    void Ozz_RigModule::ScheduleSample(  
            Handle<SkeletonTag> skeleton,
            Handle<ClipTag> clip, 
            Handle<CacheTag> cache, 
            Handle<PoseTag> poseOut,
            float time) { m_SampleTasks.push_back({ skeleton, clip, cache, time, poseOut }); }

    void Ozz_RigModule::ScheduleBlend(
        Handle<PoseTag> poseA,
        Handle<PoseTag> poseB,
        Handle<PoseTag> poseOut,
        float alpha) { m_BlendTasks.push_back({ BlendMode::Lerp, poseA, poseB, Handle<MaskTag>::MakeInvalid(), alpha, poseOut }); }
    
    
    void Ozz_RigModule::ScheduleAdditive(
        Handle<PoseTag> poseBase,
        Handle<PoseTag> poseAdditive,
        Handle<PoseTag> poseOut,
        float weight) { m_BlendTasks.push_back({ BlendMode::Additive, poseBase, poseAdditive, Handle<MaskTag>::MakeInvalid(), weight, poseOut }); }
    

    void Ozz_RigModule::ScheduleLayeredBlend(
        Handle<PoseTag> poseA,
        Handle<PoseTag> poseB,
        Handle<MaskTag> mask,
        Handle<PoseTag> poseOut) { m_BlendTasks.push_back({ BlendMode::Layered, poseA, poseB, mask, 0.0f, poseOut }); }
    

    void Ozz_RigModule::ScheduleTwoBoneIK(const TwoBoneIKSpec& spec)
    {
        IKTask task;
        task.mode = IKMode::TwoBone;
        task.TBspec = spec;
        m_IKTasks.push_back(task);
    }

    void Ozz_RigModule::ScheduleLookAt(const LookAtSpec& spec)
    {
        IKTask task;
        task.mode   = IKMode::LookAt;
        task.LAspec = spec;
        m_IKTasks.push_back(task);
    }


    void Ozz_RigModule::ScheduleFinalize(
        Handle<SkeletonTag> skeleton,
        Handle<PoseTag> pose) { m_FinalizeTasks.push_back({ skeleton, pose }); }
    


    void Ozz_RigModule::ExecuteSampleTasks()
    {
        uint32_t total = (uint32_t)m_SampleTasks.size();
        if (total == 0) return;

        uint32_t chunkSize = 16;
        JobSystem::ParallelFor(total, chunkSize, m_SampleTasks.data(), AE_MAKE_LAMBDA((this), (auto& task), auto,
            SampleClipIntoPose(task);
        ));
    }

    void Ozz_RigModule::ExecuteBlendTasks()
    {
        uint32_t total = (uint32_t)m_BlendTasks.size();
        if (total == 0) return;

        uint32_t chunkSize = 16;
        JobSystem::ParallelFor(total, chunkSize, m_BlendTasks.data(), AE_MAKE_LAMBDA((this), (auto& task), auto,
            BlendPoses(task);
        ));
    }

    void Ozz_RigModule::ExecuteIKTasks()
    {
        uint32_t total = (uint32_t)m_IKTasks.size();
        if (total == 0) return;

        uint32_t chunkSize = 16;
        JobSystem::ParallelFor(total, chunkSize, m_IKTasks.data(), AE_MAKE_LAMBDA((this), (auto& task), auto,
            if (task.mode == IKMode::TwoBone) ApplyTwoBoneIK(task);
            else ApplyLookAt(task);
        ));
    }

    void Ozz_RigModule::ExecuteFinalizeTasks()
    {
        uint32_t total = (uint32_t)m_FinalizeTasks.size();
        if (total == 0) return;

        uint32_t chunkSize = 16;
        JobSystem::ParallelFor(total, chunkSize, m_FinalizeTasks.data(), AE_MAKE_LAMBDA((this), (auto& task), auto,
            FinalizePose(task);
        ));
    }
    
    void Ozz_RigModule::ProcessTasks() 
    {
        ExecuteSampleTasks();    
        ExecuteBlendTasks();     
        ExecuteIKTasks();       
        ExecuteFinalizeTasks();  
    }

    void Ozz_RigModule::ClearTasks()
    {
        m_SampleTasks.clear();
        m_BlendTasks.clear();
        m_IKTasks.clear();
        m_FinalizeTasks.clear();
    }

    // queries

    int Ozz_RigModule::GetJointIndex(Handle<SkeletonTag> skeleton, const std::string& name) const
    {
        auto it = m_SkeletonPool.GetResource(skeleton);
        if (!it) return -1;

        const auto names = it->data->joint_names();

        for (int i = 0; i < (int)names.size(); i++)
            if (names[i] == name) return i;
        return -1;
    }

    std::string Ozz_RigModule::GetJointName(Handle<SkeletonTag> skeleton, int index) const 
    {
        auto it = m_SkeletonPool.GetResource(skeleton);
        if (!it) return "";

        const auto names = it->data->joint_names();
        if (index >= (int)names.size()) return "";
        return std::string(names[index]);
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

    bool Ozz_RigModule::GetIBM(Handle<SkeletonTag> skeleton, int boneIndex, glm::mat4& out) const
    {
        const auto* it = m_SkeletonPool.GetResource(skeleton);
        if (!it) return false;
        if (boneIndex < 0 || (size_t)boneIndex >= it->orderedIBMs.size()) return false;
        out = it->orderedIBMs[boneIndex];
        return true;
    }

    std::tuple<const glm::mat4*, size_t> Ozz_RigModule::GetPose(Handle<PoseTag> pose)
    {
        auto* it = m_PosePool.GetResource(pose);
        if (!it || it->finalMats.empty())
        {
            //AE_CORE_ERROR("Pose not found");
            return {nullptr, 0};
        }
        return {it->finalMats.data(), it->finalMats.size()};
    }

    // helper 

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