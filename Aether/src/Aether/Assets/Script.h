#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Container/Handle.h"
#include "Aether/Scripting/ScriptEngine.h"
#include <string>

namespace Aether {

    class Script : public Asset
    {
    public:
        Script(const std::string& source)
        {
            handle = ScriptEngine::LoadScriptSource(source);
        }
        Script(Handle<Bytecode> script)
        {
            handle = script;
        }
        virtual ~Script() = default;

        Handle<Bytecode> GetHandle() { return handle; }

        template<typename... Args>
        static Ref<Bytecode> Create(Args&&... args)
        {
            return CreateRef<Bytecode>(std::forward<Args>(args)...);
        }
    private:
        Handle<Bytecode> handle;

        static Scope<Script> CreateImpl(const std::string& source) { return CreateScope<Script>(source);}
        static Scope<Script> CreateImpl(Handle<Bytecode> script) { return CreateScope<Script>(script);}
        friend class AssetManager;
    };
}