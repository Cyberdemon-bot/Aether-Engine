#pragma once

namespace Aether {
    class ScriptEngine;

    struct Binder
    {
        static void RegisterMathBindings(ScriptEngine* engine);
        static void RegisterEnumBindings(ScriptEngine* engine);
        static void RegisterCoreBindings(ScriptEngine* engine);
        static void RegisterAsyncBindings(ScriptEngine* engine);
        static void RegisterSceneBindings(ScriptEngine* engine);
        static void RegisterPhysicsBindings(ScriptEngine* engine);
    };
}