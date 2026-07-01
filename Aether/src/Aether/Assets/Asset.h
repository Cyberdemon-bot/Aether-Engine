#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"

namespace Aether {

    enum class AssetType : uint32_t 
    {
        None = 0,
        Mesh,
        Material,
        Sheet,
        Skeleton,
        Clip,
        Script,
        Count 
    };

    class Asset
    {
    public:
        UUID id;
        bool loaded = false;
        virtual ~Asset() = default;
    };
}