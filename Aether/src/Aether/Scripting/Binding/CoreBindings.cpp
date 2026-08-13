#include "Aether/Core/JobSystem.h"
#include "Aether/Physics/PhysicsSystem.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Scripting/ScriptGlue.h"
#include "Aether/Scripting/Bindings.h"

namespace Aether {
    void Binder::RegisterCoreBindings(ScriptEngine* engine)
    {
        engine->BindType<TransformBinding>();
        engine->BindType<LightParamBinding>();   
        engine->BindModule<InputBinding>("Input"); 
    }
}