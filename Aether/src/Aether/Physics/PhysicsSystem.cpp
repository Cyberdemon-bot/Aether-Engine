#include "Aether/Physics/PhysicsSystem.h"

namespace Aether {

    Scope<PhysicsAPI> PhysicsSystem::s_PhysicsAPI = PhysicsAPI::Create();
    
}