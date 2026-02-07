#include "aepch.h"
#include "Aether/Animation/AnimationManager.h"
#include "Aether/Animation/SkeletalAnimationSystem.h"

namespace Aether {
    AnimationManager& AnimationManager::GetInstance()
    {
        static AnimationManager instance;
        return instance;
    }

    void AnimationManager::Init()
    {
        auto& instance = GetInstance();
        instance.RegisterSystem(AnimationType::Skeletal, SkeletalAnimationSystem::Create());
        // .... systems

        AE_CORE_INFO("AnimationManager initialized");
    }

    void AnimationManager::Shutdown()
    {
        auto& instance = GetInstance();
        for (auto& [type, systemPtr] : instance.m_Systems)
        {
            if (!systemPtr) continue;
            switch(type)
            {
                case AnimationType::Skeletal: 
                {  
                    auto* refPtr = static_cast<Ref<SkeletalAnimationSystem>*>(systemPtr);
                    delete refPtr; 
                    break;
                }  
                case AnimationType::Rigid:
                case AnimationType::None:
                    break;
            }
        }
        instance.m_Systems.clear();
    }

    void AnimationManager::Update(Timestep ts)
    {
        auto& instance = GetInstance();
        for (auto& [type, systemPtr] : instance.m_Systems) 
        {
            if (!systemPtr) continue;
            AE_CORE_ERROR("Crashed here!");
            switch(type)
            {
                case AnimationType::Skeletal: 
                {
                    auto* refPtr = static_cast<Ref<SkeletalAnimationSystem>*>(systemPtr);
                    (*refPtr)->Update(ts);
                    break;
                }
                case AnimationType::Rigid:
                case AnimationType::None:
                    break;
            }
        }
    }

    bool AnimationManager::HasSystem(AnimationType type)
    {
        auto& instance = GetInstance();
        return instance.m_Systems.find(type) != instance.m_Systems.end();
    }
}
