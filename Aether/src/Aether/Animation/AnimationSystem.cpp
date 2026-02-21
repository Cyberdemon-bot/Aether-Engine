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

    void AnimationSystem::Update(Timestep ts)
    {
        auto& instance = GetInstance();
        for (const auto& [type, module] : instance.m_Modules) module->Update(ts);
    }
}