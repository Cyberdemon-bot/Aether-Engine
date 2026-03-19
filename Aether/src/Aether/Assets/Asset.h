#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"

namespace Aether {
    enum class AssetType
    {
        Mesh, Material, Sound
    };

    struct AssetHandle
    {
        int index = -1, generation = -1;
        bool IsValid() const { return index >= 0 && generation >= 0; }
        void MakeInvalid() { index = -1, generation = -1; }
    };

    class Asset
    {
    public:
        UUID id;
        virtual ~Asset() = default;
        virtual const AssetType GetAssetType() const = 0;
    };
}