#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include "Aether/Importer/AnimationParser.h"
#include "Aether/Core/Timestep.h"
#include <glm/glm.hpp>
#include <vector>

namespace Aether {

    class SkeletalAnimationSystem
    {
    public:
        virtual ~SkeletalAnimationSystem() = default;

        virtual void RegisterSkeleton(const SkeletonCreateInfo& data, UUID id) = 0;
        virtual void RegisterClip(const AnimationClipCreateInfo& data, UUID id) = 0;

        virtual void CreateAnimator(UUID animatorID, UUID skeletonID) = 0;
        virtual void DestroyAnimator(UUID animatorID) = 0;
        
        virtual void BindClip(UUID animatorID, UUID clipID) = 0;
        virtual void SetSpeed(UUID animatorID, float speed) = 0;
        virtual void SetLoop(UUID animatorID, bool loop) = 0;
        virtual void SetTime(UUID animatorID, float time) = 0;

        virtual void Play(UUID animatorID) = 0;
        virtual void Pause(UUID animatorID) = 0;
        virtual void Stop(UUID animatorID) = 0;

        virtual void Update(Timestep ts) = 0;
        virtual void RequestMatrices(UUID animatorID) = 0;
        virtual void ProcessRequests() = 0;
        virtual const std::vector<glm::mat4>& GetMatrices(UUID animatorID) = 0;

        virtual bool IsPlaying(UUID animatorID) const = 0;
        virtual float GetCurrentTime(UUID animatorID) const = 0;
        virtual float GetDuration(UUID animatorID) const = 0;
        virtual float GetSpeed(UUID animatorID) const = 0;

        static Ref<SkeletalAnimationSystem> Create();
    };

}