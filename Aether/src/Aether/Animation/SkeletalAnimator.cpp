#include "aepch.h"
#include "SkeletalAnimator.h"

namespace Aether {

    SkeletalAnimator::SkeletalAnimator(const SkeletonSpec& spec)
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

    void SkeletalAnimator::Play(const Clip* clip, bool loop)
    {
        if (!clip) return;
        
        currentClip = clip;
        currentTime = 0.0f;
        isPlaying = true;
        isPaused = false;
        isLooping = loop;
    }

    void SkeletalAnimator::Stop()
    {
        isPlaying = false;
        isPaused = false;
        currentTime = 0.0f;
        currentClip = nullptr;
        ResetToBindPose();
    }

    void SkeletalAnimator::Pause()
    {
        isPaused = true;
    }

    void SkeletalAnimator::Resume()
    {
        isPaused = false;
    }

    void SkeletalAnimator::Update(Timestep ts)
    {
        if (!isPlaying || isPaused || !currentClip) return;

        float deltaTime = ts.GetSeconds();
        currentTime += deltaTime * playbackSpeed;

        if (currentTime >= currentClip->Durations)
        {
            if (isLooping)  currentTime = fmod(currentTime, currentClip->Durations);
            else
            {
                currentTime = currentClip->Durations;
                isPlaying = false;
            }
        }
        else if (currentTime < 0.0f)
        {
            if (isLooping) currentTime = currentClip->Durations + fmod(currentTime, currentClip->Durations);
            else
            {
                currentTime = 0.0f;
                isPlaying = false;
            }
        }

        localTransforms = bindPose;

        for (const Channel& channel : currentClip->Channels)
            if (channel.boneIdx >= 0 && channel.boneIdx < localTransforms.size())
                localTransforms[channel.boneIdx] = channel.Sample(currentTime);

        ComputeFinalMatrices(); 
    }

    void SkeletalAnimator::ComputeFinalMatrices()
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

    void SkeletalAnimator::ResetToBindPose()
    {
        localTransforms = bindPose; 
        ComputeFinalMatrices();
    }

    void SkeletalAnimatorLibrary::Init()
    {
        GetAnimators().reserve(128);
        AE_CORE_INFO("AnimatorLibrary initialized");
    }

    void SkeletalAnimatorLibrary::Shutdown()
    {
        GetAnimators().clear();
    }

    void SkeletalAnimatorLibrary::Add(Ref<SkeletalAnimator> obj, UUID id)
    {
        auto& animators = GetAnimators();
        if (animators.find(id) != animators.end())
        {
            AE_CORE_ERROR("Skeletal Animator Library: ID already exists");
            return;
        }

        if (!obj)
        {
            AE_CORE_ERROR("Skeletal Animator Library: Cannot add null obj");
            return;
        }
        animators[id] = obj;
    }

    Ref<SkeletalAnimator> SkeletalAnimatorLibrary::Get(UUID id)
    {
        auto& animators = GetAnimators();
        if (animators.find(id) != animators.end()) 
            return animators[id];

        AE_CORE_WARN("Animator Library: Animator ID not found!");
        return nullptr;
    }

    bool SkeletalAnimatorLibrary::Exists(UUID id)
    {
        auto& animators = GetAnimators();
        return animators.find(id) != animators.end();
    }

    std::unordered_map<UUID, Ref<SkeletalAnimator>>& SkeletalAnimatorLibrary::GetAnimators()
    {
        static std::unordered_map<UUID, Ref<SkeletalAnimator>> s_Animators;
        return s_Animators;
    }
}