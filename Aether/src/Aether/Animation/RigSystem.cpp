#include "Aether/Animation/RigSystem.h"
#include "Platform/Ozz/Ozz_AnimationSystem.h"

namespace Aether {

    Ref<RigSystem> RigSystem::Create()
    {
        return CreateRef<Ozz_AnimationSystem>();
    }

}