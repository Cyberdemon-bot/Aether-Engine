#include "aepch.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"

namespace Aether {
    AnimationSystem& AnimationSystem::GetInstance()
    {
        static AnimationSystem instance;
        return instance;
    }

    void AnimationSystem::Init()
    {
        AddModule<RigModule>();
        AE_CORE_INFO("AnimationSystem initialized");
    }

    void AnimationSystem::Shutdown()
    {
        auto& instance = GetInstance();
        instance.m_Modules.clear();
    }
}