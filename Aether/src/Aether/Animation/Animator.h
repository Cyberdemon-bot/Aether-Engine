#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Animation/AnimationClip.h"
#include "Aether/Core/Timestep.h"
#include "Aether/Core/UUID.h"
#include <glm/glm.hpp>
#include <vector>

namespace Aether {
    struct AnimatorSpec
    {
        const std::vector<int32_t>& parentIndices;
        const std::vector<glm::mat4>& InverseBindMatrices;
        const std::vector<glm::mat4>& localBindPose;
    };
    
    class AETHER_API Animator
    {
    public:
        Animator(const AnimatorSpec& spec);

        void Play(const AnimationClip* clip, bool loop = true);
        void Stop();
        void Pause();
        void Resume();

        void Update(Timestep ts);

        const std::vector<glm::mat4>& GetFinalMatrices() { return finalBoneMatrices; }
        bool IsPlaying() { return isPlaying && !isPaused; }
        float GetCurrentTime() const { return currentTime; }
        size_t GetBoneCount() { return parentIndices.size(); }
        const AnimationClip* GetCurrentClip() { return currentClip; }

        void SetPlaybackSpeed(float speed) { playbackSpeed = speed; }
        float GetPlaybackSpeed() const { return playbackSpeed; }

    private:
        void ComputeFinalMatrices();
        void ResetToBindPose();

        std::vector<int32_t> parentIndices;
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<glm::mat4> localTransforms;
        std::vector<glm::mat4> finalBoneMatrices;
        std::vector<glm::mat4> bindPose;

        const AnimationClip* currentClip;
        float currentTime;
        float playbackSpeed;
        bool isPlaying;
        bool isPaused;
        bool isLooping;
    };

    class AETHER_API AnimatorLibrary
    {
    public:
        static void Init();
        static void Shutdown();

        static Ref<Animator> Load(AnimatorSpec spec, UUID id);
        static Ref<Animator> Get(UUID id);

        static bool Exists(UUID id);
    private:
        static std::unordered_map<UUID, Ref<Animator>>& GetAnimators();
    };
}   




