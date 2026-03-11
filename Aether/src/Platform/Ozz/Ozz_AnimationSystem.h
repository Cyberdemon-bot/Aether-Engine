#pragma once

#include "Aether/Animation/RigModule.h"
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

    class Ozz_AnimationSystem : public RigModule
    {
    public:
        Ozz_AnimationSystem();
        virtual ~Ozz_AnimationSystem() override;

        virtual void RegisterSkeleton(const RigCreateInfo& data, UUID id) override;
        virtual void RegisterClip(const ClipCreateInfo& data, UUID id, UUID skeletonID) override;

        virtual void CreateAnimator(UUID animatorID, UUID rigID, const std::vector<UUID>& clipIDs = {}) override;
        virtual void CloneAnimator(UUID animatorID, UUID sample) override;
        virtual void DestroyAnimator(UUID animatorID) override;
        
        virtual void AddClip(UUID animatorID, UUID clipID) override;
        virtual void BindClip(UUID animatorID, uint32_t idx) override;
        virtual void BindClip(UUID animatorID, UUID clipID) override;

        virtual void SetSpeed(UUID animatorID, float speed) override;
        virtual void SetLoop(UUID animatorID, bool loop) override;
        virtual void SetTime(UUID animatorID, float time) override;

        virtual void Play(UUID animatorID) override;
        virtual void Pause(UUID animatorID) override;
        virtual void Stop(UUID animatorID) override;

        virtual void Update(Timestep ts) override;
        virtual void RequestMatrices(UUID animatorID) override;
        virtual void ProcessRequests() override;
        virtual const std::vector<glm::mat4>& GetMatrices(UUID animatorID) override;

        virtual bool IsPlaying(UUID animatorID) const override;
        virtual float GetPlayBackTime(UUID animatorID) const override;
        virtual float GetDuration(UUID animatorID) const override;
        virtual float GetSpeed(UUID animatorID) const override;
        virtual glm::mat4 GetRootMat(UUID animatorID) const override;

        virtual bool HasAnimator(UUID animatorID) const override;
        virtual uint32_t GetClipCount(UUID animatorID) const override;
        virtual int GetCurrentClipIndex(UUID animatorID) const override;
        virtual std::vector<UUID> GetClips(UUID animatorID) const override;

    private:
        struct OzzSkeleton
        {
            ozz::unique_ptr<ozz::animation::Skeleton> skeleton;
            RigCreateInfo sourceData; 
        };

        struct OzzClip
        {
            ozz::unique_ptr<ozz::animation::Animation> animation;
            ClipCreateInfo sourceData;  
        };

        struct OzzAnimator
        {
            UUID rigID;
            int currentClip = -1;
            std::vector<UUID> clipIDs;
            
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
        ozz::unique_ptr<ozz::animation::Skeleton> ConvertToOzzSkeleton(const RigCreateInfo& data);
        ozz::unique_ptr<ozz::animation::Animation> ConvertToOzzAnimation(const ClipCreateInfo& data, int numJoints);
        void ConvertOzzMatricesToGlm(const ozz::vector<ozz::math::Float4x4>& ozzMats, std::vector<glm::mat4>& glmMats);
        void ConvertOzzMatrixToGlm(const ozz::math::Float4x4& ozzMat, glm::mat4& glmMat) const;

    private:
        std::unordered_map<UUID, OzzSkeleton> m_Skeletons;
        std::unordered_map<UUID, OzzClip> m_Clips;
        std::unordered_map<UUID, Scope<OzzAnimator>> m_Animators;
        
        std::vector<UUID> m_RequestQueue;
        std::mutex m_Mutex;
    };

}