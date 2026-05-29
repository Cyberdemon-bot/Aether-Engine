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
        BindType<TransformComponentBinding>();
        BindType<LightParamBinding>();   
        BindType<SceneCameraBinding>();
        BindType<ScriptSelfBinding>();
        BindType<SceneBinding>();
        BindType<EventManagerBinding>();
        BindType<CollisionBinding>();
        BindType<RaycastHitBinding>();
        BindType<PhysicsBinding>();
        BindType<AsyncBinding>();
    }

    void ScriptEngine::ImportNativeFunc(const std::string& name, Delegate<ScriptValue(const ScriptArgs&)> func)
    {
        auto& instance = GetInstance();
        auto& lua = instance.LuaState.lua;
        sol::table native = lua["Native"].get_or_create<sol::table>();
        instance.m_NativeFuncs.push_back({name, func});
        native.set_function(name, [func, &lua](sol::variadic_args va) -> sol::object
        {
            ScriptArgs args;
            for (const auto& v : va)
            {
                sol::object obj = v;
                args.Pushback(FromSolObject(obj));
            }
            
            ScriptValue result = func(args);
            return ToSolObject(lua, result);
        });
    }
}