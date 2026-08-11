#include "Aether/Core/JobSystem.h"
#include "Aether/Physics/PhysicsSystem.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Scripting/ScriptGlue.h"
#include "Aether/Scripting/Bindings.h"

namespace Aether {
    void Binder::RegisterPhysicsBindings(ScriptEngine* engine)
    {
        engine->BindType<PhysicsBinding>();
        engine->BindType<CollisionBinding>();
        engine->BindType<RaycastResultBinding>();
    }
}