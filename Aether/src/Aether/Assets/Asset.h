#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"

namespace Aether {
    enum class AssetType
    {
        Mesh, Material, Texture
    };

    class Asset
    {
    public:
        UUID id;
        virtual ~Asset() = default;
        virtual const AssetType GetAssetType() const = 0;
    };
}