#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"

namespace Aether {

    class Asset
    {
    public:
        UUID id;
        virtual ~Asset() = default;
    };
}