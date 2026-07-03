#include "aepch.h"
#include "Aether/Core/ServiceManager.h"
#include "Aether/Core/Log.h"

namespace Aether {
    
    std::array<void*, static_cast<size_t>(ServiceType::Count)> ServiceManager::s_Services;

    void ServiceManager::Init()
    {
        s_Services.fill(nullptr);
        AE_CORE_INFO("Service Manager initialized with O(1) Enum Mapping Architecture.");
    }

    void ServiceManager::Shutdown()
    {
        s_Services.fill(nullptr);
    }
}