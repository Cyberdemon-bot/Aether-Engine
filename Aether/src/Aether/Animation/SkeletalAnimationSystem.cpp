#include "Aether/Animation/SkeletalAnimationSystem.h"
#include "Platform/Ozz/Ozz_AnimationSystem.h"

namespace Aether {

    Ref<SkeletalAnimationSystem> SkeletalAnimationSystem::Create()
    {
        return CreateRef<Ozz_AnimationSystem>();
    }

}