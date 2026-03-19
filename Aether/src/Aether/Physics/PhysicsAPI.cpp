#include "Platform/JoltPhys/Jolt_PhysicsAPI.h"
#include "Aether/Core/Assert.h"

namespace Aether {
    PhysicsAPI::API PhysicsAPI::s_API = PhysicsAPI::API::JoltPhys;

    Scope<PhysicsAPI> PhysicsAPI::Create()
    {
        switch (s_API)
		{
			case PhysicsAPI::API::None:    AE_CORE_ASSERT(false, "PhysicsAPI::None is currently not supported!"); return nullptr;
			case PhysicsAPI::API::JoltPhys:  return CreateScope<Jolt_PhysicsAPI>();
		}

		AE_CORE_ASSERT(false, "Unknown PhysicsAPI!");
		return nullptr;
    }
}