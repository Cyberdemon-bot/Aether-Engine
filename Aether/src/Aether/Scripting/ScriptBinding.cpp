#include "aepch.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Scripting/ScriptGlue.h"

namespace Aether {
    void ScriptEngine::RegisterBinding()
    {
        BindEnum<Key::KeyCode>("Key");
        BindEnum<Mouse::MouseCode>("Mouse");
        BindEnum<CollisionType>("CollisionType");
        BindEnum<LightType>("LightType"); 
        BindEnum<SceneCamera::ProjectionType>("CameraProjection");
        BindType<Vec3Binding>("Math");
        BindType<QuatBinding>("Math");
        BindModule<MathBinding>("Math");
        BindModule<InputBinding>("Input"); 
        BindType<TransformBinding>();
        BindType<LightParamBinding>();   
        BindType<SceneCameraBinding>();
        BindType<ScriptSelfBinding>();
        BindType<SceneBinding>();
        BindType<EventManagerBinding>();
        BindType<CollisionBinding>();
        BindType<RaycastHitBinding>();
        BindType<PhysicsBinding>();
        BindType<CoroutineBinding>();
    }

    void ScriptEngine::ImportNativeFunc(std::string_view name, Delegate<ScriptTable(const ScriptTable&)> func)
    {
        auto& lua = LuaState.lua;
        sol::table native = lua["Native"].get_or_create<sol::table>();
        m_NativeFuncs.push_back({std::string(name), func});
        uint64_t index = static_cast<uint64_t>(m_NativeFuncs.size() - 1);
        
        native.set_function(name, [func, &lua](sol::variadic_args va) -> sol::object
        {
            ScriptTable args;
            for (const auto& v : va)
            {
                sol::object obj = v;
                args.Pushback(ScriptTable::FromSolObject(obj));
            }
            
            ScriptTable result = func(args);
            return ScriptTable::ToSolObject(lua, result);
        });

        sol::table iref = native["IRef"].get_or_create<sol::table>();
        iref[name] = index;
    }
}