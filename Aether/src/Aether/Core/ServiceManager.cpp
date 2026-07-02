#include "aepch.h"
#include "Aether/Core/ServiceManager.h"

namespace Aether {
    std::vector<void*> ServiceManager::s_Services;

    void ServiceManager::Init()
    {
        s_Services.reserve(16);
        AE_CORE_INFO("Service manager initialized");
    }

    void ServiceManager::Shutdown()
    {
        s_Services.clear();
    }
}