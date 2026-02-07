#pragma once

#include "Aether/Animation/SkeletalAnimationSystem.h"
#include "Aether/Core/Timestep.h"
#include <unordered_map>

namespace Aether {
    enum class AnimationType
    {
        None = 0, Skeletal, Rigid
    };

    class AnimationManager
    {
    public: 
        static void Init();
        static void Shutdown();
        static void Update(Timestep ts);
        static bool HasSystem(AnimationType type);

        template<typename T>
        static Ref<T> GetSystem(AnimationType type);

        template<typename T>
        static void RegisterSystem(AnimationType type, Ref<T> system);
    private:
        AnimationManager() = default;
        static AnimationManager& GetInstance();
        std::unordered_map<AnimationType, void*> m_Systems;
    };

    template<typename T>
    inline Ref<T> AnimationManager::GetSystem(AnimationType type)
    {
        auto& instance = GetInstance();
        auto it = instance.m_Systems.find(type);
        if (it != instance.m_Systems.end())
        {
            return *static_cast<Ref<T>*>(it->second);
        }
        return nullptr;
    }

    template<typename T>
    inline void AnimationManager::RegisterSystem(AnimationType type, Ref<T> system)
    {
        auto& instance = GetInstance();
        auto* refCopy = new Ref<T>(system);
        instance.m_Systems[type] = refCopy;
    }
}