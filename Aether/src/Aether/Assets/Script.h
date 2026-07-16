#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"
#include "Aether/Scripting/ScriptEngine.h"
#include "Aether/Core/ServiceManager.h"
#include <string>

namespace Aether {

    struct Script : public Asset
    {
        Script(const std::string& source)
        {
            m_Handle = ServiceManager::GetService<ScriptEngine>()->LoadScriptSource(source);
        }

        Script(Handle<Bytecode> script)
        {
            m_Handle = script;
        }

        Handle<Bytecode> m_Handle;
    };
}