#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"

namespace Aether {
    struct AssetHandle
    {
        uint32_t index = UINT32_MAX;
        uint32_t generation = 0;
        bool IsValid() const { return index != UINT32_MAX; }
        void MakeInvalid() { index = UINT32_MAX; }
    };

    class Asset
    {
    public:
        UUID id;
        virtual ~Asset() = default;
    };
}