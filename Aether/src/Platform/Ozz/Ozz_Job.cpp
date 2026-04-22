#include "Platform/Ozz/Ozz_RigModule.h"
#include <ozz/animation/runtime/blending_job.h>
#include <ozz/animation/runtime/ik_two_bone_job.h>
#include <ozz/animation/runtime/ik_aim_job.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/simd_quaternion.h>
#include <ozz/animation/runtime/local_to_model_job.h>

namespace Aether {

    void Ozz_RigModule::SampleClipIntoPose(const SampleTask& task)
    {
        auto* skeleton = m_SkeletonPool.GetResource(task.skeleton);
        auto* clip = m_ClipPool.GetResource(task.clip);
        auto* cache = m_CachePool.GetResource(task.cache);
        auto* pose = m_PosePool.GetResource(task.poseOut);
        if (!skeleton || !clip || !cache || !pose) return;

        float duration = clip->data->duration();
        float ratio = 0.0f;
        if (duration > 0.0f)
        {
            ratio = std::fmod(task.time, duration) / duration;
            if (ratio < 0.0f) ratio += 1.0f;
        }

        pose->localTransforms.resize(skeleton->data->num_soa_joints());

        ozz::animation::SamplingJob samplingJob;
        samplingJob.animation = clip->data.get();
        samplingJob.context = cache->data.get();
        samplingJob.ratio = ratio;
        samplingJob.output = ozz::make_span(pose->localTransforms);
        samplingJob.Run()
    }

    void Ozz_RigModule::BlendPoses(const BlendTask& task)
    {
        auto* poseA = m_PosePool.GetResource(task.poseA);
        auto* poseB = m_PosePool.GetResource(task.poseB);
        auto* poseOut = m_PosePool.GetResource(task.poseOut);
        if (!poseA || !poseB || !poseOut) return;

        int num = (int)poseA->localTransforms.size();
        poseOut->localTransforms.resize(num);

        if (task.mode == BlendMode::Lerp)
        {
            ozz::animation::BlendingJob::Layer layers[2];
            layers[0].transform = ozz::make_span(poseA->localTransforms);
            layers[0].weight = 1.0f - task.alpha;
            layers[1].transform = ozz::make_span(poseB->localTransforms);
            layers[1].weight = task.alpha;

            ozz::animation::BlendingJob blendJob;
            blendJob.threshold = 0.1f;
            blendJob.layers = ozz::make_span(layers);
            blendJob.output = ozz::make_span(poseOut->localTransforms);
            blendJob.Run();
        }
        else if (task.mode == BlendMode::Additive)
        {
            ozz::animation::BlendingJob::Layer baseLayer;
            baseLayer.transform = ozz::make_span(poseA->localTransforms);
            baseLayer.weight    = 1.0f;

            ozz::animation::BlendingJob::Layer addLayer;
            addLayer.transform = ozz::make_span(poseB->localTransforms);
            addLayer.weight = task.alpha;

            ozz::animation::BlendingJob blendJob;
            blendJob.threshold = 0.1f;
            blendJob.layers = ozz::span<ozz::animation::BlendingJob::Layer>(&baseLayer, 1);
            blendJob.additive_layers = ozz::span<ozz::animation::BlendingJob::Layer>(&addLayer, 1);
            blendJob.output = ozz::make_span(poseOut->localTransforms);
            blendJob.Run();
        }
        else if (task.mode == BlendMode::Layered)
        {
            auto* mask = m_MaskPool.GetResource(task.mask);
            if (!mask) return;

            int numSoa = num;
            int numJoints = (int)mask->weights.size();
            ozz::vector<ozz::math::SimdFloat4> jointWeightsA(numSoa);
            ozz::vector<ozz::math::SimdFloat4> jointWeightsB(numSoa);

            for (int i = 0; i < numSoa; i++)
            {
                float w[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
                for (int k = 0; k < 4; k++)
                {
                    int j = i * 4 + k;
                    if (j < numJoints) w[k] = mask->weights[j];
                }
                jointWeightsB[i] = ozz::math::simd_float4::Load(w[0], w[1], w[2], w[3]);
                jointWeightsA[i] = ozz::math::simd_float4::Load(1.0f - w[0], 1.0f - w[1], 1.0f - w[2], 1.0f - w[3]);
            }

            ozz::animation::BlendingJob::Layer layers[2];
            layers[0].transform = ozz::make_span(poseA->localTransforms);
            layers[0].weight = 1.0f;
            layers[0].joint_weights = ozz::make_span(jointWeightsA);
            layers[1].transform = ozz::make_span(poseB->localTransforms);
            layers[1].weight = 1.0f;
            layers[1].joint_weights = ozz::make_span(jointWeightsB);

            ozz::animation::BlendingJob blendJob;
            blendJob.threshold = 0.1f;
            blendJob.layers = ozz::make_span(layers);
            blendJob.output = ozz::make_span(poseOut->localTransforms);
            blendJob.Run();
        }
    }

    void Ozz_RigModule::ApplyTwoBoneIK(const IKTask& task)
    {
        const auto& spec = task.TBspec;

        auto* skeleton = m_SkeletonPool.GetResource(spec.Skeleton);
        auto* pose = m_PosePool.GetResource(spec.Pose);
        if (!skeleton || !pose) return;

        int numJoints = skeleton->data->num_joints();
        thread_local ozz::vector<ozz::math::Float4x4> modelMats;
        modelMats.resize(numJoints);

        ozz::animation::LocalToModelJob l2mJob;
        l2mJob.skeleton = skeleton->data.get();
        l2mJob.input = ozz::make_span(pose->localTransforms);
        l2mJob.output = ozz::make_span(modelMats);
        if (!l2mJob.Run()) return;

        auto toSimd = [](const glm::vec3& v) {
            return ozz::math::simd_float4::Load(v.x, v.y, v.z, 0.0f);
        };

        ozz::animation::IKTwoBoneJob ikJob;
        ikJob.target = toSimd(spec.Target);
        ikJob.pole_vector = toSimd(spec.Pole);
        ikJob.weight = spec.Weight;
        ikJob.start_joint = &modelMats[spec.Root];
        ikJob.mid_joint = &modelMats[spec.Mid];
        ikJob.end_joint = &modelMats[spec.End];

        ozz::math::SimdQuaternion startCorrection;
        ozz::math::SimdQuaternion midCorrection;
        ikJob.start_joint_correction = &startCorrection;
        ikJob.mid_joint_correction = &midCorrection;

        if (!ikJob.Run()) return;

        auto applyCorrection = [&](int jointIdx, const ozz::math::SimdQuaternion& correction)
        {
            int soaIdx = jointIdx / 4;
            int lane   = jointIdx % 4;

            ozz::math::SoaTransform& soa = pose->localTransforms[soaIdx];

            ozz::math::SimdFloat4 cols[4] = {
                soa.rotation.x,  
                soa.rotation.y, 
                soa.rotation.z,  
                soa.rotation.w  
            };
            ozz::math::SimdFloat4 rows[4]; 
            ozz::math::Transpose4x4(cols, rows);
            ozz::math::SimdQuaternion localQ;
            localQ.xyzw = rows[lane];

            ozz::math::SimdQuaternion newQ =
                ozz::math::NormalizeSafe(correction * localQ,
                                        ozz::math::SimdQuaternion::identity());

            rows[lane] = newQ.xyzw;

            ozz::math::Transpose4x4(rows, cols);
            soa.rotation.x = cols[0];
            soa.rotation.y = cols[1];
            soa.rotation.z = cols[2];
            soa.rotation.w = cols[3];
        };

        applyCorrection(spec.Root, startCorrection);
        applyCorrection(spec.Mid,  midCorrection);
    }

    void Ozz_RigModule::ApplyLookAt(const IKTask& task)
    {
        const auto& spec = task.LAspec;

        auto* skeleton = m_SkeletonPool.GetResource(spec.Skeleton);
        auto* pose     = m_PosePool.GetResource(spec.Pose);
        if (!skeleton || !pose) return;

        int numJoints = skeleton->data->num_joints();
        thread_local ozz::vector<ozz::math::Float4x4> modelMats;
        modelMats.resize(numJoints);

        ozz::animation::LocalToModelJob l2mJob;
        l2mJob.skeleton = skeleton->data.get();
        l2mJob.input    = ozz::make_span(pose->localTransforms);
        l2mJob.output   = ozz::make_span(modelMats);
        if (!l2mJob.Run()) return;

        auto toSimd = [](const glm::vec3& v) {
            return ozz::math::simd_float4::Load(v.x, v.y, v.z, 0.0f);
        };

        ozz::animation::IKAimJob aimJob;
        aimJob.target           = toSimd(spec.Target);
        aimJob.forward          = toSimd(spec.Forward);
        aimJob.up               = toSimd(spec.Up);
        aimJob.joint            = &modelMats[spec.Bone];
        aimJob.weight           = spec.Weight;
        aimJob.pole_vector      = toSimd(spec.Up);

        ozz::math::SimdQuaternion correction;
        aimJob.joint_correction = &correction;

        if (!aimJob.Run()) return;
        int soaIdx = spec.Bone / 4;
        int lane   = spec.Bone % 4;

        ozz::math::SoaTransform& soa = pose->localTransforms[soaIdx];

        ozz::math::SimdFloat4 cols[4] = {
            soa.rotation.x, 
            soa.rotation.y,  
            soa.rotation.z,  
            soa.rotation.w  
        };
        ozz::math::SimdFloat4 rows[4];
        ozz::math::Transpose4x4(cols, rows);

        ozz::math::SimdQuaternion localQ;
        localQ.xyzw = rows[lane];
        ozz::math::SimdQuaternion newQ =
            ozz::math::NormalizeSafe(correction * localQ,
                                    ozz::math::SimdQuaternion::identity());
        rows[lane] = newQ.xyzw;
        ozz::math::Transpose4x4(rows, cols);

        soa.rotation.x = cols[0];
        soa.rotation.y = cols[1];
        soa.rotation.z = cols[2];
        soa.rotation.w = cols[3];
    }

    void Ozz_RigModule::FinalizePose(const FinalizeTask& task)
    {
        auto* skeleton = m_SkeletonPool.GetResource(task.skeleton);
        auto* pose = m_PosePool.GetResource(task.pose);
        if (!skeleton || !pose) return;

        int numJoints = skeleton->data->num_joints();

        thread_local ozz::vector<ozz::math::Float4x4> modelMats;
        modelMats.resize(numJoints);

        ozz::animation::LocalToModelJob l2mJob;
        l2mJob.skeleton = skeleton->data.get();
        l2mJob.input = ozz::make_span(pose->localTransforms);
        l2mJob.output = ozz::make_span(modelMats);
        if (!l2mJob.Run()) return;

        pose->finalMats.resize(numJoints);
        for (int i = 0; i < numJoints; i++)
        {
            ConvertOzzMatrixToGlm(modelMats[i], pose->finalMats[i]);
            pose->finalMats[i] = pose->finalMats[i] * skeleton->orderedIBMs[i];
        }
    }
}