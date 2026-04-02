#pragma once

#include "Aether/Core/Timestep.h"
#include "Aether/Core/Base.h"

#include <string>
#include <algorithm>
#include <vector>

namespace Aether {

    class AnimationModule
    {
    public:
        virtual ~AnimationModule() = default;
    private:
        virtual const char* GetName() const = 0;
    };

    class AETHER_API AnimationSystem
    {
    public: 
        static void Init();
        static void Shutdown();

        template <typename T>
        static void AddModule() 
        {
            auto& instance = GetInstance();
            Ref<T> module = T::Create();
            const auto& name = module->GetName();

            auto it = std::find_if(instance.m_Modules.begin(), instance.m_Modules.end(), 
                [name](const auto& pair) { return pair.first == name; });
            if (it != instance.m_Modules.end()) it->second = module;
            else instance.m_Modules.push_back({name, module});
        }

        template <typename T>
        static Ref<T> GetModule() 
        {
            auto& instance = GetInstance();
            const auto& name = T::ModuleName();
            auto it = std::find_if(instance.m_Modules.begin(), instance.m_Modules.end(), 
                [name](const auto& pair) { return pair.first == name; });
            if (it != instance.m_Modules.end()) 
                return std::static_pointer_cast<T>(it->second);
            return nullptr;
        }

    private:
        AnimationSystem() = default;
        static AnimationSystem& GetInstance();
        std::vector<std::pair<std::string, Ref<AnimationModule>>> m_Modules;
    };
}