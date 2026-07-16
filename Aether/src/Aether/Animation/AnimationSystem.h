#pragma once

#include <tuple>
#include <vector>
#include <algorithm>
#include "Aether/Core/Base.h"
#include "Aether/Animation/RigModule.h"

namespace Aether {

    enum class ModuleType
    {
        None, Rig
    };

    template<typename T> struct GetModuleType;
    template<> struct GetModuleType<RigModule> { static constexpr ModuleType value = ModuleType::Rig; };

    class AETHER_API AnimationSystem
    {
    public: 
        void Init();
        void Shutdown();

        template <typename T>
        T* GetModule() 
        {
            return std::get<Scope<T>>(m_Modules).get();
        }
    private:
        std::tuple<Scope<RigModule>> m_Modules;
    };
}