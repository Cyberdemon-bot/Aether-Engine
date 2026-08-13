#include "Aether/Core/JobSystem.h"
#include "Aether/Physics/PhysicsSystem.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Scripting/ScriptGlue.h"
#include "Aether/Scripting/Bindings.h"

namespace Aether {
    void Binder::RegisterAsyncBindings(ScriptEngine* engine)
    {
        engine->BindType<JobBinding>();
        engine->BindType<PromiseBinding>();
        engine->BindType<CoroutineBinding>();
    }
}