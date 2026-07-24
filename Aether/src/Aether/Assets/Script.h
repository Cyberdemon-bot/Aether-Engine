#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"

namespace Aether {

    struct Bytecode;
    struct Script : public Asset
    {
        Script() = default;
        Handle<Bytecode> m_Handle;
    };
}