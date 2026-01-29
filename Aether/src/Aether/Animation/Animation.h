#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Animation/AnimationClip.h"
#include "Aether/Core/UUID.h"
#include <unordered_map>

namespace Aether {
    
    struct Animation
    {
        AnimationClip Clip;

        Animation() = default;
        Animation(const AnimationClip& clip)
            : Clip(clip) {}
    };

    class AETHER_API AnimationLibrary
    {
    public:
        static void Init();
        static void Shutdown();

        static Ref<Animation> Load(const AnimationClip& clip, UUID id);
        static Ref<Animation> Get(UUID id);
        static bool Exists(UUID id);

    private:
        static std::unordered_map<UUID, Ref<Animation>>& GetAnimations();
    };
}