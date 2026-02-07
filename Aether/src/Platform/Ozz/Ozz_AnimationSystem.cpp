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
        AE_CORE_INFO("Ozz_AnimationSystem initialized");
    }

    Ozz_AnimationSystem::~Ozz_AnimationSystem()
    {
        m_Animators.clear();
        m_Clips.clear();
        m_Skeletons.clear();
    }

    // ===== Asset Registration =====

    void Ozz_AnimationSystem::RegisterSkeleton(const SkeletonCreateInfo& data, UUID id)
    {
        OzzSkeleton ozzSkel;
        ozzSkel.sourceData = data;
        ozzSkel.skeleton = ConvertToOzzSkeleton(data);

        if (!ozzSkel.skeleton) 
        {
            AE_CORE_ERROR("Failed to build ozz skeleton from {0} (Builder returned null)", data.DebugName);
            return;
        }
        
        if (!ozzSkel.skeleton->num_joints())
        {
            AE_CORE_ERROR("Failed to create ozz skeleton from {0}", data.DebugName);
            return;
        }

        AE_CORE_INFO("Registered skeleton: {0} with {1} joints", 
            data.DebugName, ozzSkel.skeleton->num_joints());
        
        m_Skeletons[id] = std::move(ozzSkel);
    }

    void Ozz_AnimationSystem::RegisterClip(const AnimationClipCreateInfo& data, UUID id)
    {
        OzzClip ozzClip;
        ozzClip.sourceData = data;
        ozzClip.animation = ConvertToOzzAnimation(data);

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


    void Ozz_AnimationSystem::CreateAnimator(UUID animatorID, UUID skeletonID)
    {
        auto skelIt = m_Skeletons.find(skeletonID);
        if (skelIt == m_Skeletons.end()) { return; }

        auto animator = CreateScope<OzzAnimator>(); 
        animator->skeletonID = skeletonID;
        
        const ozz::animation::Skeleton& skeleton = *skelIt->second.skeleton; 
        int numJoints = skeleton.num_joints();
        animator->localTransforms.resize(skeleton.num_soa_joints());
        animator->modelMatrices.resize(numJoints);
        animator->finalMatrices.resize(numJoints, glm::mat4(1.0f));
        animator->samplingContext.Resize(numJoints);
        m_Animators[animatorID] = std::move(animator);
        
        AE_CORE_INFO("Created animator {0}...", (uint64_t)animatorID);
    }

    void Ozz_AnimationSystem::DestroyAnimator(UUID animatorID)
    {
        auto it = m_Animators.find(animatorID);
        if (it != m_Animators.end())
        {
            m_Animators.erase(it);
            AE_CORE_INFO("Destroyed animator {0}", (uint64_t)animatorID);
        }
    }


    void Ozz_AnimationSystem::BindClip(UUID animatorID, UUID clipID)
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

        animIt->second->clipID = clipID;
        animIt->second->currentTime = 0.0f;
        animIt->second->dirty = true;
        
        AE_CORE_INFO("Bound clip {0} to animator {1}", (uint64_t)clipID, (uint64_t)animatorID);
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


    void Ozz_AnimationSystem::Play(UUID animatorID)
    {
        auto it = m_Animators.find(animatorID);
        if (it != m_Animators.end())
        {
            it->second->isPlaying = true;
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
        AE_CORE_ERROR("Crashed here!");
        for (auto& [id, animator] : m_Animators)
        {
            if (!animator->isPlaying) continue;
            if (animator->clipID == UUID(0)) continue;

            auto clipIt = m_Clips.find(animator->clipID);
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

        AE_CORE_TRACE("Processing {0} animation requests", requestsCopy.size());
        
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

    // ===== Query State =====

    bool Ozz_AnimationSystem::IsPlaying(UUID animatorID) const
    {
        auto it = m_Animators.find(animatorID);
        return (it != m_Animators.end()) ? it->second->isPlaying : false;
    }

    float Ozz_AnimationSystem::GetCurrentTime(UUID animatorID) const
    {
        auto it = m_Animators.find(animatorID);
        return (it != m_Animators.end()) ? it->second->currentTime : 0.0f;
    }

    float Ozz_AnimationSystem::GetDuration(UUID animatorID) const
    {
        auto it = m_Animators.find(animatorID);
        if (it == m_Animators.end() || it->second->clipID == UUID(0))
            return 0.0f;
        
        auto clipIt = m_Clips.find(it->second->clipID);
        return (clipIt != m_Clips.end()) ? clipIt->second.animation->duration() : 0.0f;
    }

    // ===== Private Methods =====

    void Ozz_AnimationSystem::CalculateMatrices(UUID animatorID)
    {
        auto animIt = m_Animators.find(animatorID);
        if (animIt == m_Animators.end()) return;

        OzzAnimator& animator = *(animIt->second);
        
        // Skip if not dirty
        if (!animator.dirty) return;
        
        // Get skeleton
        auto skelIt = m_Skeletons.find(animator.skeletonID);
        if (skelIt == m_Skeletons.end()) return;
        
        const ozz::animation::Skeleton& skeleton = *(skelIt->second.skeleton);
        
        // Get animation
        auto clipIt = m_Clips.find(animator.clipID);
        if (clipIt == m_Clips.end())
        {
            // No animation bound, use bind pose
            for (size_t i = 0; i < animator.finalMatrices.size(); i++)
            {
                animator.finalMatrices[i] = glm::mat4(1.0f);
            }
            animator.dirty = false;
            return;
        }
        
        const ozz::animation::Animation& animation = *(clipIt->second.animation);
        
        // Sample animation
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
        
        // Convert local transforms to model space
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
        const auto& ibms = skelIt->second.sourceData.IBM;
        int numJoints = (int)animator.modelMatrices.size();
        if (ibms.size() != numJoints)
        {
            AE_CORE_ERROR("IBM count mismatch! IBMs: {0}, Joints: {1}", 
                ibms.size(), numJoints);
            animator.dirty = false;
            return;  
        }
        for (int i = 0; i < numJoints; ++i) animator.finalMatrices[i] = animator.finalMatrices[i] * ibms[i];
        
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
        AE_CORE_TRACE("Submitted {0} animation calculation jobs", animatorIDs.size());
    }

    // ===== Conversion Helpers =====

    ozz::unique_ptr<ozz::animation::Skeleton> Ozz_AnimationSystem::ConvertToOzzSkeleton(const SkeletonCreateInfo& data)
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
        
        // Build hierarchy recursively
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
        
        // Build runtime skeleton
        SkeletonBuilder builder;
        return builder(rawSkeleton);
    }

    ozz::unique_ptr<ozz::animation::Animation> Ozz_AnimationSystem::ConvertToOzzAnimation(const AnimationClipCreateInfo& data)
    {
        using namespace ozz::animation::offline;
        
        RawAnimation rawAnim;
        rawAnim.duration = data.Duration;
        rawAnim.tracks.resize(data.Tracks.size());
        
        for (size_t i = 0; i < data.Tracks.size(); i++)
        {
            const auto& srcTrack = data.Tracks[i];
            RawAnimation::JointTrack& dstTrack = rawAnim.tracks[i];
            
            // Translation keys
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
            
            // Rotation keys
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
            
            // Scale keys
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
        
        // Build runtime animation
        AnimationBuilder builder;
        return builder(rawAnim);
    }

    void Ozz_AnimationSystem::ConvertOzzMatricesToGlm(const ozz::vector<ozz::math::Float4x4>& ozzMats, std::vector<glm::mat4>& glmMats)
    {
        glmMats.resize(ozzMats.size());
        
        for (size_t i = 0; i < ozzMats.size(); i++)
        {
            const ozz::math::Float4x4& src = ozzMats[i];
            glm::mat4& dst = glmMats[i];
            

            ozz::math::StorePtrU(src.cols[0], glm::value_ptr(dst[0]));
            ozz::math::StorePtrU(src.cols[1], glm::value_ptr(dst[1]));
            ozz::math::StorePtrU(src.cols[2], glm::value_ptr(dst[2]));
            ozz::math::StorePtrU(src.cols[3], glm::value_ptr(dst[3]));
        }
    }

}