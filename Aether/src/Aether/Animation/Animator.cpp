#include "aepch.h"
#include "Animator.h"

namespace Aether {

    Animator::Animator(const AnimatorSpec& spec)
        : parentIndices(spec.parentIndices)
        , inverseBindMatrices(spec.InverseBindMatrices)
        , currentClip(nullptr)
        , currentTime(0.0f)
        , playbackSpeed(1.0f)
        , isPlaying(false)
        , isPaused(false)
        , isLooping(true)
    {
        size_t boneCount = parentIndices.size();
        localTransforms.resize(boneCount, glm::mat4(1.0f));
        bindPose.resize(boneCount, glm::mat4(1.0f));
        finalBoneMatrices.resize(boneCount, glm::mat4(1.0f));
        bindPose = spec.localBindPose;
        
        ResetToBindPose();
    }

    void Animator::Play(const AnimationClip* clip, bool loop)
    {
        if (!clip) return;
        
        currentClip = clip;
        currentTime = 0.0f;
        isPlaying = true;
        isPaused = false;
        isLooping = loop;
    }

    void Animator::Stop()
    {
        isPlaying = false;
        isPaused = false;
        currentTime = 0.0f;
        currentClip = nullptr;
        ResetToBindPose();
    }

    void Animator::Pause()
    {
        isPaused = true;
    }

    void Animator::Resume()
    {
        isPaused = false;
    }

    void Animator::Update(Timestep ts)
    {
        if (!isPlaying || isPaused || !currentClip) return;

        float deltaTime = ts.GetSeconds();
        currentTime += deltaTime * playbackSpeed;

        if (currentTime >= currentClip->duration)
        {
            if (isLooping)  currentTime = fmod(currentTime, currentClip->duration);
            else
            {
                currentTime = currentClip->duration;
                isPlaying = false;
            }
        }
        else if (currentTime < 0.0f)
        {
            if (isLooping) currentTime = currentClip->duration + fmod(currentTime, currentClip->duration);
            else
            {
                currentTime = 0.0f;
                isPlaying = false;
            }
        }

        localTransforms = bindPose;

        for (const BoneChannel& channel : currentClip->channels)
            if (channel.boneIdx >= 0 && channel.boneIdx < localTransforms.size())
                localTransforms[channel.boneIdx] = channel.Sample(currentTime);

        ComputeFinalMatrices(); 
    }

    void Animator::ComputeFinalMatrices()
    {
        std::vector<glm::mat4> worldTransforms(parentIndices.size());
        
        for (size_t i = 0; i < parentIndices.size(); i++)
        {
            if (parentIndices[i] == -1) worldTransforms[i] = localTransforms[i];
            else
            {
                int32_t parentIdx = parentIndices[i];
                worldTransforms[i] = worldTransforms[parentIdx] * localTransforms[i];
            }
        }

        for (size_t i = 0; i < parentIndices.size(); i++)
        {
            finalBoneMatrices[i] = worldTransforms[i] * inverseBindMatrices[i];
        }
    }

    void Animator::ResetToBindPose()
    {
        localTransforms = bindPose; 
        ComputeFinalMatrices();
    }

    void AnimatorLibrary::Init()
    {
        GetAnimators().reserve(128);
        AE_CORE_INFO("AnimatorLibrary initialized");
    }

    void AnimatorLibrary::Shutdown()
    {
        GetAnimators().clear();
    }

    Ref<Animator> AnimatorLibrary::Load(AnimatorSpec spec, UUID id)
    {
        auto& animators = GetAnimators();
        if(animators.find(id) != animators.end()) 
            return animators[id];

        auto animator = CreateRef<Animator>(spec);

        animators[id] = animator;
        return animator;
    }

    Ref<Animator> AnimatorLibrary::Get(UUID id)
    {
        auto& animators = GetAnimators();
        if (animators.find(id) != animators.end()) 
            return animators[id];

        AE_CORE_WARN("Animator Library: Animator ID not found!");
        return nullptr;
    }

    bool AnimatorLibrary::Exists(UUID id)
    {
        auto& animators = GetAnimators();
        return animators.find(id) != animators.end();
    }

    std::unordered_map<UUID, Ref<Animator>>& AnimatorLibrary::GetAnimators()
    {
        static std::unordered_map<UUID, Ref<Animator>> s_Animators;
        return s_Animators;
    }
}