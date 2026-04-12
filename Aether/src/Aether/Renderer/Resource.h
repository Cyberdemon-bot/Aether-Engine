#pragma once

#include "Aether/Core/Base.h"

namespace Aether {
    struct ResourceHandle
    {
        uint32_t index = UINT32_MAX;
        uint32_t generation = 0;
        bool IsValid() const { return index != UINT32_MAX; }
    };

    class Resource
    {
    public:
        virtual ~Resource() = default;
    };
}