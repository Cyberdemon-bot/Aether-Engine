#include "Aether/Core/JobSystem.h"
#include "Aether/Physics/PhysicsSystem.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Scripting/ScriptGlue.h"
#include "Aether/Scripting/Bindings.h"

namespace Aether {
    void Binder::RegisterEnumBindings(ScriptEngine* engine)
    {
        engine->BindEnum<Key::KeyCode>("Key");
        engine->BindEnum<Mouse::MouseCode>("Mouse");

        engine->BindEnum<LightType>("LightType"); 
        engine->BindEnum<CollisionType>("CollisionType");
        engine->BindEnum<SceneCamera::ProjectionType>("CameraProjection");
    }
}