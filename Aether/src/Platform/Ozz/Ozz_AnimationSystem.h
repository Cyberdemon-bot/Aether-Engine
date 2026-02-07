#pragma once

#include "Aether/Animation/SkeletalAnimationSystem.h"
#include <unordered_map>
#include <mutex>

// Ozz-animation includes
#include <ozz/animation/runtime/animation.h>
#include <ozz/animation/runtime/skeleton.h>
#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>
#include <ozz/base/memory/unique_ptr.h>

namespace Aether {

    class Ozz_AnimationSystem : public SkeletalAnimationSystem
    {
    public:
        Ozz_AnimationSystem();
        virtual ~Ozz_AnimationSystem() override;

        // ===== Asset Registration =====
        void RegisterSkeleton(const SkeletonCreateInfo& data, UUID id) override;
        void RegisterClip(const AnimationClipCreateInfo& data, UUID id) override;

        // ===== Animator Management =====
        void CreateAnimator(UUID animatorID, UUID skeletonID) override;
        void DestroyAnimator(UUID animatorID) override;
        
        // ===== Configuration =====
        void BindClip(UUID animatorID, UUID clipID) override;
        void SetSpeed(UUID animatorID, float speed) override;
        void SetLoop(UUID animatorID, bool loop) override;
        void SetTime(UUID animatorID, float time) override;

        // ===== Playback Control =====
        void Play(UUID animatorID) override;
        void Pause(UUID animatorID) override;
        void Stop(UUID animatorID) override;

        // ===== Update & Calculation =====
        void Update(Timestep ts) override;
        void RequestMatrices(UUID animatorID) override;
        void ProcessRequests() override;
        const std::vector<glm::mat4>& GetMatrices(UUID animatorID) override;

        // ===== Query State =====
        bool IsPlaying(UUID animatorID) const override;
        float GetCurrentTime(UUID animatorID) const override;
        float GetDuration(UUID animatorID) const override;

    private:
        struct OzzSkeleton
        {
            ozz::unique_ptr<ozz::animation::Skeleton> skeleton;
            SkeletonCreateInfo sourceData; 
        };

        struct OzzClip
        {
            ozz::unique_ptr<ozz::animation::Animation> animation;
            AnimationClipCreateInfo sourceData;  
        };

        struct OzzAnimator
        {
            UUID skeletonID;
            UUID clipID;
            
            float currentTime = 0.0f;
            float playbackSpeed = 1.0f;
            bool isPlaying = false;
            bool loop = true;
            bool dirty = false;
            
            ozz::animation::SamplingJob::Context samplingContext;
            ozz::vector<ozz::math::SoaTransform> localTransforms;
            ozz::vector<ozz::math::Float4x4> modelMatrices;
            
            std::vector<glm::mat4> finalMatrices;
        };

    private:
        void CalculateMatrices(UUID animatorID);
        void CalculateMatricesBatch(const std::vector<UUID>& animatorIDs);
        
        // Conversion helpers
        ozz::unique_ptr<ozz::animation::Skeleton> ConvertToOzzSkeleton(const SkeletonCreateInfo& data);
        ozz::unique_ptr<ozz::animation::Animation> ConvertToOzzAnimation(const AnimationClipCreateInfo& data);
        void ConvertOzzMatricesToGlm(const ozz::vector<ozz::math::Float4x4>& ozzMats, 
                                      std::vector<glm::mat4>& glmMats);

    private:
        std::unordered_map<UUID, OzzSkeleton> m_Skeletons;
        std::unordered_map<UUID, OzzClip> m_Clips;
        std::unordered_map<UUID, Scope<OzzAnimator>> m_Animators;
        
        std::vector<UUID> m_RequestQueue;
        std::mutex m_Mutex;
    };

}