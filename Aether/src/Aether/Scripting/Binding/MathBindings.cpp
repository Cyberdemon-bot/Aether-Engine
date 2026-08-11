#include "Aether/Core/JobSystem.h"
#include "Aether/Physics/PhysicsSystem.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Scripting/ScriptGlue.h"
#include "Aether/Scripting/Bindings.h"

namespace Aether {
    void Binder::RegisterMathBindings(ScriptEngine* engine)
    {
        engine->BindType<U64Binding>();
        engine->BindType<Vec3Binding>("Math");
        engine->BindType<QuatBinding>("Math");
        engine->BindModule<MathBinding>("Math");
    }
}