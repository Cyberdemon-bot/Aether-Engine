#pragma once

#include "Aether/Core/Timestep.h"
#include "Aether/Core/Base.h"
#include <unordered_map>
#include <typeindex>

namespace Aether {

    class AnimationModule
    {
    public:
        virtual ~AnimationModule() = default;
        virtual void Update(Timestep ts) = 0;
    };

    class AnimationSystem
    {
    public: 
        static void Init();
        static void Shutdown();
        static void Update(Timestep ts);

        template <typename T>
        static void AddModule() 
        {
            auto& instance = GetInstance();
            instance.m_Modules[std::type_index(typeid(T))] = T::Create();
        }

        template <typename T>
        static Ref<T> GetModule() 
        {
            auto& instance = GetInstance();
            auto it = instance.m_Modules.find(std::type_index(typeid(T)));
            if (it != instance.m_Modules.end()) 
            {
                return std::static_pointer_cast<T>(it->second);
            }
            return nullptr;
        }
    private:
        AnimationSystem() = default;
        static AnimationSystem& GetInstance();
        std::unordered_map<std::type_index, Ref<AnimationModule>> m_Modules;
    };
}