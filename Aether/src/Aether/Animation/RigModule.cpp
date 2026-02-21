#include "Aether/Animation/RigModule.h"
#include "Platform/Ozz/Ozz_AnimationSystem.h"

namespace Aether {

    Ref<RigModule> RigModule::Create()
    {
        return CreateRef<Ozz_AnimationSystem>();
    }

}