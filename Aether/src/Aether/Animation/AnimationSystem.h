#pragma once

#include "Aether/Core/Timestep.h"
#include "Aether/Core/Base.h"
#include <unordered_map>
#include <string>

namespace Aether {

    class AnimationModule
    {
    public:
        virtual ~AnimationModule() = default;
        virtual void Update(Timestep ts) = 0;
        virtual const char* GetName() const = 0;
    };

    class AETHER_API AnimationSystem
    {
    public: 
        static void Init();
        static void Shutdown();
        static void Update(Timestep ts);

        template <typename T>
        static void AddModule() 
        {
            auto& instance = GetInstance();
            Ref<T> module = T::Create();
            instance.m_Modules[module->GetName()] = module;
        }

        template <typename T>
        static Ref<T> GetModule() 
        {
            auto& instance = GetInstance();
            auto it = instance.m_Modules.find(T::ModuleName());
            if (it != instance.m_Modules.end()) 
                return std::static_pointer_cast<T>(it->second);
            return nullptr;
        }

    private:
        AnimationSystem() = default;
        static AnimationSystem& GetInstance();
        std::unordered_map<std::string, Ref<AnimationModule>> m_Modules;
    };
}