#pragma once

#include "Aether/Core/TypeRegistry.h"
#include <vector>

namespace Aether {
    class ServiceManager
    {
    public:
        static void Init();
        static void Shutdown();

        template<typename T>
        static void Provide(T* service)
        {
            uint32_t idx = TypeRegistry::GetCode<T>();
            if (idx >= s_Services.size()) s_Services.resize(idx + 1, nullptr);
            s_Services[idx] = service;
        }

        template<typename T>
        static T* GetService()
        {
            uint32_t idx = TypeRegistry::GetCode<T>();
            if (idx < s_Services.size()) return static_cast<T*>(s_Services[idx]);
            return nullptr;
        }
    private:
        static std::vector<void*> s_Services;
    };
}