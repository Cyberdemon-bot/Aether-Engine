#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Core/ServiceManager.h"
#include <string>

namespace Aether {

    class Script : public Asset
    {
    public:
        Script(const std::string& source)
        {
            handle = ServiceManager::GetService<ScriptEngine>()->LoadScriptSource(source);
        }
        Script(Handle<Bytecode> script)
        {
            handle = script;
        }
        virtual ~Script() = default;

        Handle<Bytecode> GetHandle() { return handle; }
    private:
        Handle<Bytecode> handle;
    };
}