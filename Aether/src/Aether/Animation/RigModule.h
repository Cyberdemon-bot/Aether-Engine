#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include "Aether/Importer/AnimationParser.h"
#include "Aether/Core/Timestep.h"
#include "Aether/Animation/AnimationSystem.h"
#include <glm/glm.hpp>
#include <vector>

namespace Aether {

    class RigModule : public AnimationModule
    {
    public:
        virtual void RegisterSkeleton(const RigCreateInfo& data, UUID id) = 0;
        virtual void RegisterClip(const ClipCreateInfo& data, UUID id, UUID skeletonID) = 0;

        virtual void CreateAnimator(UUID animatorID, UUID skeletonID, const std::vector<UUID>& clipIDs = {}) = 0;
        virtual void CloneAnimator(UUID animatorID, UUID sample) = 0;
        virtual void DestroyAnimator(UUID animatorID) = 0;
        
        virtual void AddClip(UUID animatorID, UUID clipID) = 0;
        virtual void BindClip(UUID animatorID, uint32_t idx) = 0;
        virtual void BindClip(UUID animatorID, UUID clipID) = 0;

        virtual void SetSpeed(UUID animatorID, float speed) = 0;
        virtual void SetLoop(UUID animatorID, bool loop) = 0;
        virtual void SetTime(UUID animatorID, float time) = 0;

        virtual void Play(UUID animatorID) = 0;
        virtual void Pause(UUID animatorID) = 0;
        virtual void Stop(UUID animatorID) = 0;

        virtual void RequestMatrices(UUID animatorID) = 0;
        virtual void ProcessRequests() = 0;
        virtual const std::vector<glm::mat4>& GetMatrices(UUID animatorID) = 0;

        virtual bool IsPlaying(UUID animatorID) const = 0;
        virtual float GetPlayBackTime(UUID animatorID) const = 0;
        virtual float GetDuration(UUID animatorID) const = 0;
        virtual float GetSpeed(UUID animatorID) const = 0;

        virtual bool HasAnimator(UUID animatorID) const = 0;
        virtual uint32_t GetClipCount(UUID animatorID) const = 0;
        virtual int GetCurrentClipIndex(UUID animatorID) const = 0;
        virtual std::vector<UUID> GetClips(UUID animatorID) const = 0;

        virtual int GetBoneIndex(UUID animatorID, const std::string& name) const = 0;
        virtual glm::mat4 GetBoneMat(UUID animatorID, uint32_t index) const = 0;
        virtual std::vector<glm::mat4> GetRestPoseMatrices(UUID rigID) const = 0;

        static const char* ModuleName() { return "RigModule"; }
        const char* GetName() const override { return "RigModule"; }

        static Ref<RigModule> Create();
    };

}