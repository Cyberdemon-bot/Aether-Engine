#include "aepch.h"
#include "Aether/Animation/AnimationSystem.h"

namespace Aether {

    void AnimationSystem::Init()
    {
        m_Modules = { RigModule::Create() };
        AE_CORE_INFO("AnimationSystem initialized");
    }

    void AnimationSystem::Shutdown()
    {
        std::apply([](auto&&... args) {
            (args.reset(), ...); 
        }, m_Modules);
    }
}