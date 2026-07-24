#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"

namespace Aether {

    class Resource;

    struct Image : public Asset
    {
        Image() = default;

        Handle<Resource> m_Handle;
    };
}