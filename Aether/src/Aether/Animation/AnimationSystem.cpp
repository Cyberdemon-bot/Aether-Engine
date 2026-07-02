#include "aepch.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"

namespace Aether {

    void AnimationSystem::Init()
    {
        AddModule<RigModule>();
        AE_CORE_INFO("AnimationSystem initialized");
    }

    void AnimationSystem::Shutdown()
    {
        m_Modules.clear();
    }
}