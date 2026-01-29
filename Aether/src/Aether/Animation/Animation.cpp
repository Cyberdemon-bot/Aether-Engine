#include "aepch.h"
#include "Animation.h"

namespace Aether {

    void AnimationLibrary::Init()
    {
        GetAnimations().reserve(128);
        AE_CORE_INFO("AnimationLibrary initialized");
    }

    void AnimationLibrary::Shutdown()
    {
        GetAnimations().clear();
    }

    Ref<Animation> AnimationLibrary::Load(const AnimationClip& clip, UUID id)
    {
        auto& animations = GetAnimations();
        if(animations.find(id) != animations.end()) 
            return animations[id];

        auto animation = CreateRef<Animation>(clip);
        animations[id] = animation;
        return animation;
    }

    Ref<Animation> AnimationLibrary::Get(UUID id)
    {
        auto& animations = GetAnimations();
        if (animations.find(id) != animations.end()) 
            return animations[id];

        AE_CORE_WARN("Animation Library: Animation ID not found!");
        return nullptr;
    }

    bool AnimationLibrary::Exists(UUID id)
    {
        auto& animations = GetAnimations();
        return animations.find(id) != animations.end();
    }

    std::unordered_map<UUID, Ref<Animation>>& AnimationLibrary::GetAnimations()
    {
        static std::unordered_map<UUID, Ref<Animation>> s_Animations;
        return s_Animations;
    }
}