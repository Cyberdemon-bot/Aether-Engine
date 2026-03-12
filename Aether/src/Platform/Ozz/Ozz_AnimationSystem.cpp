#include "Platform/Ozz/Ozz_AnimationSystem.h"
#include "Aether/Core/JobSystem.h"
#include "Aether/Core/Log.h"

#include <ozz/animation/offline/raw_skeleton.h>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/offline/raw_animation.h>
#include <ozz/animation/offline/animation_builder.h>
#include <ozz/base/maths/simd_math.h> 
#include <glm/gtc/type_ptr.hpp>

namespace Aether {

    Ozz_AnimationSystem::Ozz_AnimationSystem()
    {
        m_Animators.reserve(128);
        m_Clips.reserve(128);
        m_Skeletons.reserve(128);
    }

    Ozz_AnimationSystem::~Ozz_AnimationSystem()
    {
        m_Animators.clear();
        m_Clips.clear();
        m_Skeletons.clear();
    }

    void Ozz_AnimationSystem::RegisterSkeleton(const RigCreateInfo& data, UUID id)
    {
        OzzSkeleton ozzSkeleton;
        ozzSkeleton.skeleton = ConvertToOzzSkeleton(data);

        if (!ozzSkeleton.skeleton) 
        {
            AE_CORE_ERROR("Failed to build ozz skeleton from {0} (Builder returned null)", data.DebugName);
            return;
        }
        
        if (!ozzSkeleton.skeleton->num_joints())
        {
            AE_CORE_ERROR("Failed to create ozz skeleton from {0}", data.DebugName);
            return;
        }

        AE_CORE_INFO("Registered skeleton: {0} with {1} joints", 
            data.DebugName, ozzSkeleton.skeleton->num_joints());

        const auto& jointNames = ozzSkeleton.skeleton->joint_names();
        ozzSkeleton.ibmRemap.resize(jointNames.size());
        for (int ozzIdx = 0; ozzIdx < (int)jointNames.size(); ozzIdx++)
        {
            for (int origIdx = 0; origIdx < (int)data.Joints.size(); origIdx++)
            {
                if (data.Joints[origIdx].Name == jointNames[ozzIdx])
                {
                    ozzSkeleton.ibmRemap[ozzIdx] = origIdx;
                    break;
                }
            }
        }
        ozzSkeleton.IBM = std::move(data.IBM);
        
        m_Skeletons[id] = std::move(ozzSkeleton);
    }

    void Ozz_AnimationSystem::RegisterClip(const ClipCreateInfo& data, UUID id, UUID skeletonID)
    {
        auto rigIt = m_Skeletons.find(skeletonID);
        if (rigIt == m_Skeletons.end()) { AE_CORE_ERROR("Skeleton not found for clip {0}", data.DebugName); return; }
        
        int numJoints = rigIt->second.skeleton->num_joints();

        OzzClip ozzClip;
        ozzClip.animation = ConvertToOzzAnimation(data, numJoints);

        if (!ozzClip.animation) 
        {
            AE_CORE_ERROR("Failed to build ozz animation from {0} (Builder returned null)", data.DebugName);
            return;
        }
        
        if (ozzClip.animation->duration() <= 0.0f)
        {
            AE_CORE_ERROR("Failed to create ozz animation from {0}", data.DebugName);
            return;
        }

        AE_CORE_INFO("Registered animation clip: {0}, duration: {1}s", 
            data.DebugName, ozzClip.animation->duration());
        
        m_Clips[id] = std::move(ozzClip);
    }

    void Ozz_AnimationSystem::CreateAnimator(UUID animatorID, UUID rigID, const std::vector<UUID>& clipIDs)
    {
        auto rigIt = m_Skeletons.find(rigID);
        if (rigIt == m_Skeletons.end()) { return; }

        auto animator = CreateScope<OzzAnimator>(); 
        animator->rigID = rigID;
        
        const ozz::animation::Skeleton& skeleton = *rigIt->second.skeleton; 
        int numJoints = skeleton.num_joints();
        animator->localTransforms.resize(skeleton.num_soa_joints());
        animator->modelMatrices.resize(numJoints);
        animator->finalMatrices.resize(numJoints, glm::mat4(1.0f));
        animator->samplingContext.Resize(numJoints);
        animator->clipIDs = clipIDs;
        m_Animators[animatorID] = std::move(animator);
    }

    void Ozz_AnimationSystem::CloneAnimator(UUID animatorID, UUID sample)
    {
        auto samIt = m_Animators.find(sample);
        if (samIt == m_Animators.end()) return;

        auto rigIt = m_Skeletons.find(samIt->second->rigID);

        auto animator = CreateScope<OzzAnimator>(); 
        animator->rigID = samIt->second->rigID;
        
        const ozz::animation::Skeleton& skeleton = *rigIt->second.skeleton; 
        int numJoints = skeleton.num_joints();
        animator->localTransforms.resize(skeleton.num_soa_joints());
        animator->modelMatrices.resize(numJoints);
        animator->finalMatrices.resize(numJoints, glm::mat4(1.0f));
        animator->samplingContext.Resize(numJoints);
        animator->clipIDs = samIt->second->clipIDs;
        m_Animators[animatorID] = std::move(animator);
    }

    void Ozz_AnimationSystem::DestroyAnimator(UUID animatorID)
    {
        auto it = m_Animators.find(animatorID);
        if (it != m_Animators.end()) m_Animators.erase(it);
    }

    void Ozz_AnimationSystem::AddClip(UUID animatorID, UUID clipID)
    {
        auto animIt = m_Animators.find(animatorID);
        if (animIt == m_Animators.end())
        {
            AE_CORE_ERROR("Animator {0} not found", (uint64_t)animatorID);
            return;
        }

        if (m_Clips.find(clipID) == m_Clips.end())
        {
            AE_CORE_ERROR("Clip {0} not found", (uint64_t)clipID);
            return;
        }

        animIt->second->clipIDs.push_back(clipID);
    }

    void Ozz_AnimationSystem::BindClip(UUID animatorID, uint32_t idx)
    {
        auto animIt = m_Animators.find(animatorID);
        if (animIt == m_Animators.end())
        {
            AE_CORE_ERROR("Animator {0} not found", (uint64_t)animatorID);
            return;
        }

        if (idx >= animIt->second->clipIDs.size())
        {
            AE_CORE_ERROR("Clip index {0} out of bound", idx);
            return;
        }

        animIt->second->currentClip = (int)idx;
        animIt->second->currentTime = 0.0f;
        animIt->second->dirty = true;
    }

    void Ozz_AnimationSystem::BindClip(UUID animatorID, UUID clipID)
    {
        auto animIt = m_Animators.find(animatorID);
        if (animIt == m_Animators.end())
        {
            AE_CORE_ERROR("Animator {0} not found", (uint64_t)animatorID);
            return;
        }

        auto& animator = animIt->second;
        int idx = -1;
        for (size_t i = 0; i < animator->clipIDs.size(); i++)
        {
            if (animator->clipIDs[i] == clipID)
            {
                idx = (int)i;
                break;
            }
        }

        if (idx == -1)
        {
            AE_CORE_ERROR("Clip {0} not found in animator {1}", (uint64_t)clipID, (uint64_t)animatorID);
            return;
        }

        animator->currentClip = idx;
        animator->currentTime = 0.0f;
        animator->dirty = true;
    }
       

    std::vector<UUID> Ozz_AnimationSystem::GetClips(UUID animatorID) const
    {
        auto animIt = m_Animators.find(animatorID);
        if (animIt == m_Animators.end())
        {
            AE_CORE_ERROR("Animator {0} not found", (uint64_t)animatorID);
            return {};
        }

        return animIt->second->clipIDs;
    }

    bool Ozz_AnimationSystem::HasAnimator(UUID animatorID) const 
    {
        auto animIt = m_Animators.find(animatorID);
        return animIt != m_Animators.end();
    }
    uint32_t Ozz_AnimationSystem::GetClipCount(UUID animatorID) const
    {
        auto animIt = m_Animators.find(animatorID);
        if (animIt == m_Animators.end())
        {
            AE_CORE_ERROR("Animator {0} not found", (uint64_t)animatorID);
            return 0;
        }
        return animIt->second->clipIDs.size();
    }

    int Ozz_AnimationSystem::GetCurrentClipIndex(UUID animatorID) const 
    {
        auto animIt = m_Animators.find(animatorID);
        if (animIt == m_Animators.end())
        {
            AE_CORE_ERROR("Animator {0} not found", (uint64_t)animatorID);
            return 0;
        }
        return animIt->second->currentClip;
    }

    std::vector<glm::mat4> Ozz_AnimationSystem::GetRestPoseMatrices(UUID rigID) const
    {
        auto it = m_Skeletons.find(rigID);
        if (it == m_Skeletons.end()) return {};

        const auto& skeleton = *it->second.skeleton;

        ozz::vector<ozz::math::Float4x4> modelMats(skeleton.num_joints());

        ozz::animation::LocalToModelJob job;
        job.skeleton = &skeleton;
        job.input    = skeleton.joint_rest_poses();
        job.output   = ozz::make_span(modelMats);
        if (!job.Run()) return {};

        std::vector<glm::mat4> result;
        result.reserve(modelMats.size());
        for (auto& m : modelMats)
        {
            glm::mat4 glmMat;
            ConvertOzzMatrixToGlm(m, glmMat);
            result.push_back(glmMat);
        }

        return result;
    }

    void Ozz_AnimationSystem::SetSpeed(UUID animatorID, float speed)
    {
        auto it = m_Animators.find(animatorID);
        if (it != m_Animators.end())
        {
            it->second->playbackSpeed = speed;
        }
    }

    void Ozz_AnimationSystem::SetLoop(UUID animatorID, bool loop)
    {
        auto it = m_Animators.find(animatorID);
        if (it != m_Animators.end())
        {
            it->second->loop = loop;
        }
    }

    void Ozz_AnimationSystem::SetTime(UUID animatorID, float time)
    {
        auto it = m_Animators.find(animatorID);
        if (it != m_Animators.end())
        {
            it->second->currentTime = time;
            it->second->dirty = true;
        }
    }

    void Ozz_AnimationSystem::Pause(UUID animatorID)
    {
        auto it = m_Animators.find(animatorID);
        if (it != m_Animators.end())
        {
            it->second->isPlaying = false;
        }
    }

    void Ozz_AnimationSystem::Play(UUID animatorID)
    {
        auto it = m_Animators.find(animatorID);
        if (it == m_Animators.end()) return;
        if (it->second->currentClip == -1) return;
        auto clipIt = m_Clips.find(it->second->clipIDs[it->second->currentClip]);
        if (clipIt == m_Clips.end()) return;
        float duration = clipIt->second.animation->duration();
        if (!it->second->loop && it->second->currentTime >= duration) it->second->currentTime = 0.0f; 
        it->second->isPlaying = true;
    }

    void Ozz_AnimationSystem::Stop(UUID animatorID)
    {
        auto it = m_Animators.find(animatorID);
        if (it != m_Animators.end())
        {
            it->second->isPlaying = false;
            it->second->currentTime = 0.0f;
            it->second->dirty = true;
        }
    }

    void Ozz_AnimationSystem::Update(Timestep ts)
    {
        for (auto& [id, animator] : m_Animators)
        {
            if (!animator->isPlaying) continue;
            if (animator->currentClip == -1) continue;

            auto clipIt = m_Clips.find(animator->clipIDs[animator->currentClip]);
            if (clipIt == m_Clips.end()) continue;

            float duration = clipIt->second.animation->duration();
            animator->currentTime += ts * animator->playbackSpeed;
            
            // Handle looping/clamping
            if (animator->loop)
            {
                while (animator->currentTime > duration)
                    animator->currentTime -= duration;
                while (animator->currentTime < 0.0f)
                    animator->currentTime += duration;
            }
            else
            {
                if (animator->currentTime > duration)
                {
                    animator->currentTime = duration;
                    animator->isPlaying = false;
                }
                else if (animator->currentTime < 0.0f)
                {
                    animator->currentTime = 0.0f;
                    animator->isPlaying = false;
                }
            }
            
            animator->dirty = true;
        }
    }

    void Ozz_AnimationSystem::RequestMatrices(UUID animatorID)
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        // Avoid duplicates
        if (std::find(m_RequestQueue.begin(), m_RequestQueue.end(), animatorID) == m_RequestQueue.end())
        {
            m_RequestQueue.push_back(animatorID);
        }
    }

    void Ozz_AnimationSystem::ProcessRequests()
    {
        std::vector<UUID> requestsCopy;
        
        {
            std::lock_guard<std::mutex> lock(m_Mutex);
            requestsCopy = m_RequestQueue;
            m_RequestQueue.clear();
        }
        
        if (requestsCopy.empty()) return;
        
        // Calculate matrices in parallel
        CalculateMatricesBatch(requestsCopy);
    }

    const std::vector<glm::mat4>& Ozz_AnimationSystem::GetMatrices(UUID animatorID)
    {
        auto it = m_Animators.find(animatorID);
        if (it != m_Animators.end())
        {
            return it->second->finalMatrices;
        }
        
        static std::vector<glm::mat4> empty;
        AE_CORE_ERROR("Animator {0} not found", (uint64_t)animatorID);
        return empty;
    }

    int Ozz_AnimationSystem::GetBoneIndex(UUID animatorID, const std::string& name) const
    {
        auto animIt = m_Animators.find(animatorID);
        if (animIt == m_Animators.end()) return -1;

        auto skelIt = m_Skeletons.find(animIt->second->rigID);
        if (skelIt == m_Skeletons.end()) return -1;

        const auto& skeleton = *skelIt->second.skeleton;
        const auto names = skeleton.joint_names();

        for (int i = 0; i < (int)names.size(); i++)
            if (names[i] == name) return i;

        return -1;
    }

    bool Ozz_AnimationSystem::IsPlaying(UUID animatorID) const
    {
        auto it = m_Animators.find(animatorID);
        return (it != m_Animators.end()) ? it->second->isPlaying : false;
    }

    float Ozz_AnimationSystem::GetPlayBackTime(UUID animatorID) const
    {
        auto it = m_Animators.find(animatorID);
        return (it != m_Animators.end()) ? it->second->currentTime : 0.0f;
    }

    float Ozz_AnimationSystem::GetSpeed(UUID animatorID) const
    {
        auto it = m_Animators.find(animatorID);
        return (it != m_Animators.end()) ? it->second->playbackSpeed : 0.0f;
    }

    bool Ozz_AnimationSystem::GetLoop(UUID animatorID) const
    {
        auto it = m_Animators.find(animatorID);
        return (it != m_Animators.end()) ? it->second->loop : false;
    }

    float Ozz_AnimationSystem::GetDuration(UUID animatorID) const
    {
        auto it = m_Animators.find(animatorID);
        if (it == m_Animators.end() || it->second->currentClip == -1)
            return 0.0f;
        
        auto clipIt = m_Clips.find(it->second->clipIDs[it->second->currentClip]);
        return (clipIt != m_Clips.end()) ? clipIt->second.animation->duration() : 0.0f;
    }

    glm::mat4 Ozz_AnimationSystem::GetBoneMat(UUID animatorID, uint32_t index) const 
    {
        auto animIt = m_Animators.find(animatorID);
        if (animIt == m_Animators.end()) return glm::mat4(1.0f);

        OzzAnimator& animator = *(animIt->second); glm::mat4 result;
        ConvertOzzMatrixToGlm(animator.modelMatrices[index], result);
        return result;
    }

    void Ozz_AnimationSystem::CalculateMatrices(UUID animatorID)
    {
        auto animIt = m_Animators.find(animatorID);
        if (animIt == m_Animators.end()) return;

        OzzAnimator& animator = *(animIt->second);
        
        if (!animator.dirty) return;
        
        auto rigIt = m_Skeletons.find(animator.rigID);
        if (rigIt == m_Skeletons.end()) return;
        
        const ozz::animation::Skeleton& skeleton = *(rigIt->second.skeleton);
        
        auto clipIt = m_Clips.find(animator.clipIDs[animator.currentClip]);
        if (clipIt == m_Clips.end())
        {
            for (size_t i = 0; i < animator.finalMatrices.size(); i++)
            {
                animator.finalMatrices[i] = glm::mat4(1.0f);
            }
            animator.dirty = false;
            return;
        }
        
        const ozz::animation::Animation& animation = *(clipIt->second.animation);
        
        ozz::animation::SamplingJob samplingJob;
        samplingJob.animation = &animation;
        samplingJob.context = &animator.samplingContext;
        samplingJob.ratio = animator.currentTime / animation.duration();
        samplingJob.output = make_span(animator.localTransforms);
        
        if (!samplingJob.Run())
        {
            AE_CORE_ERROR("Ozz sampling job failed for animator {0}", (uint64_t)animatorID);
            return;
        }
        
        ozz::animation::LocalToModelJob localToModelJob;
        localToModelJob.skeleton = &skeleton;
        localToModelJob.input = make_span(animator.localTransforms);
        localToModelJob.output = make_span(animator.modelMatrices);
        
        if (!localToModelJob.Run())
        {
            AE_CORE_ERROR("Ozz local-to-model job failed for animator {0}", (uint64_t)animatorID);
            return;
        }

        ConvertOzzMatricesToGlm(animator.modelMatrices, animator.finalMatrices);
        const auto& ibms = rigIt->second.IBM;
        int numJoints = (int)animator.modelMatrices.size();
        if (ibms.size() != numJoints)
        {
            AE_CORE_ERROR("IBM count mismatch! IBMs: {0}, Joints: {1}", 
                ibms.size(), numJoints);
            animator.dirty = false;
            return;  
        }
        for (int i = 0; i < numJoints; ++i)
        {
            int origIdx = rigIt->second.ibmRemap[i];
            animator.finalMatrices[i] = animator.finalMatrices[i] * ibms[origIdx];
        }
        
        animator.dirty = false;
    }

    void Ozz_AnimationSystem::CalculateMatricesBatch(const std::vector<UUID>& animatorIDs)
    {
        if (animatorIDs.empty()) return;

        for (const UUID& id : animatorIDs)
        {
            JobSystem::SubmitJob([this, id]() {
                CalculateMatrices(id);
            });
        }
        
        JobSystem::WaitAll();
    }


    ozz::unique_ptr<ozz::animation::Skeleton> Ozz_AnimationSystem::ConvertToOzzSkeleton(const RigCreateInfo& data)
    {
        using namespace ozz::animation::offline;
        
        RawSkeleton rawSkeleton;
        int rootIndex = -1;
        for (int i = 0; i < (int)data.Joints.size(); i++) {
            if (data.Joints[i].ParentIndex == -1) {
                rootIndex = i;
                break; 
            }
        }

        if (rootIndex == -1) return nullptr;

        rawSkeleton.roots.resize(1);
        
        std::function<void(int, RawSkeleton::Joint&)> buildHierarchy;
        buildHierarchy = [&](int parentIdx, RawSkeleton::Joint& outJoint) 
        {
            const auto& srcJoint = data.Joints[parentIdx];
            outJoint.name = srcJoint.Name;

            outJoint.transform.translation = ozz::math::Float3(srcJoint.Translation.x, srcJoint.Translation.y, srcJoint.Translation.z);
            outJoint.transform.rotation = ozz::math::Quaternion(srcJoint.Rotation.x, srcJoint.Rotation.y, srcJoint.Rotation.z, srcJoint.Rotation.w);
            outJoint.transform.scale = ozz::math::Float3(srcJoint.Scale.x, srcJoint.Scale.y, srcJoint.Scale.z);

            for (int i = 0; i < (int)data.Joints.size(); i++)
            {
                if (data.Joints[i].ParentIndex == parentIdx)
                {
                    outJoint.children.resize(outJoint.children.size() + 1);
                    buildHierarchy(i, outJoint.children.back());
                }
            }
        };
        
        buildHierarchy(rootIndex, rawSkeleton.roots[0]);

        if (!rawSkeleton.Validate()) {
            AE_CORE_ERROR("Skeleton validation failed!");
            return nullptr;
        }
        
        SkeletonBuilder builder;
        return builder(rawSkeleton);
    }

    ozz::unique_ptr<ozz::animation::Animation> Ozz_AnimationSystem::ConvertToOzzAnimation(const ClipCreateInfo& data, int numJoints)
    {
        using namespace ozz::animation::offline;
        
        RawAnimation rawAnim;
        rawAnim.duration = data.Duration;
        rawAnim.tracks.resize(numJoints);
        
        for (size_t i = 0; i < data.Tracks.size(); i++)
        {
            const auto& srcTrack = data.Tracks[i];
            int jointIdx = srcTrack.JointIndex;
            if (jointIdx < 0 || jointIdx >= numJoints) continue;

            RawAnimation::JointTrack& dstTrack = rawAnim.tracks[jointIdx];
            
            for (size_t k = 0; k < srcTrack.TranslationTimes.size(); k++)
            {
                RawAnimation::TranslationKey key;
                key.time = srcTrack.TranslationTimes[k];
                key.value = ozz::math::Float3(
                    srcTrack.TranslationValues[k].x,
                    srcTrack.TranslationValues[k].y,
                    srcTrack.TranslationValues[k].z
                );
                dstTrack.translations.push_back(key);
            }
            
            for (size_t k = 0; k < srcTrack.RotationTimes.size(); k++)
            {
                RawAnimation::RotationKey key;
                key.time = srcTrack.RotationTimes[k];
                ozz::math::Quaternion quat(
                    srcTrack.RotationValues[k].x,
                    srcTrack.RotationValues[k].y,
                    srcTrack.RotationValues[k].z,
                    srcTrack.RotationValues[k].w
                );
                key.value = ozz::math::NormalizeSafe(quat, ozz::math::Quaternion::identity());
                dstTrack.rotations.push_back(key);
            }
            
            for (size_t k = 0; k < srcTrack.ScaleTimes.size(); k++)
            {
                RawAnimation::ScaleKey key;
                key.time = srcTrack.ScaleTimes[k];
                key.value = ozz::math::Float3(
                    srcTrack.ScaleValues[k].x,
                    srcTrack.ScaleValues[k].y,
                    srcTrack.ScaleValues[k].z
                );
                dstTrack.scales.push_back(key);
            }
        }
        
        AnimationBuilder builder;
        return builder(rawAnim);
    }

    void Ozz_AnimationSystem::ConvertOzzMatrixToGlm(const ozz::math::Float4x4& ozzMat, glm::mat4& glmMat) const
    {
        ozz::math::StorePtrU(ozzMat.cols[0], glm::value_ptr(glmMat[0]));
        ozz::math::StorePtrU(ozzMat.cols[1], glm::value_ptr(glmMat[1]));
        ozz::math::StorePtrU(ozzMat.cols[2], glm::value_ptr(glmMat[2]));
        ozz::math::StorePtrU(ozzMat.cols[3], glm::value_ptr(glmMat[3]));
    }

    void Ozz_AnimationSystem::ConvertOzzMatricesToGlm(const ozz::vector<ozz::math::Float4x4>& ozzMats, std::vector<glm::mat4>& glmMats)
    {
        glmMats.resize(ozzMats.size());
        
        for (size_t i = 0; i < ozzMats.size(); i++)
        {
            const ozz::math::Float4x4& src = ozzMats[i];
            glm::mat4& dst = glmMats[i];
            ConvertOzzMatrixToGlm(src, dst);
        }
    }

}