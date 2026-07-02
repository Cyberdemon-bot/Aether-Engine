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
        void Init();
        void Shutdown();

        template <typename T>
        void AddModule() 
        {
            Ref<T> module = T::Create();
            const auto& name = module->GetName();

            auto it = std::find_if(m_Modules.begin(), m_Modules.end(), 
                [name](const auto& pair) { return pair.first == name; });
            if (it != m_Modules.end()) it->second = module;
            else m_Modules.push_back({name, module});
        }

        template <typename T>
        Ref<T> GetModule() 
        {
            const auto& name = T::ModuleName();
            auto it = std::find_if(m_Modules.begin(), m_Modules.end(), 
                [name](const auto& pair) { return pair.first == name; });
            if (it != m_Modules.end()) 
                return std::static_pointer_cast<T>(it->second);
            return nullptr;
        }

    private:
        std::vector<std::pair<std::string, Ref<AnimationModule>>> m_Modules;
    };
}