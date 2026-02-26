#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Resources/Mesh.h"
#include "Aether/Resources/Material.h"

namespace Aether {
    class Model : public Asset
    {
        Model(Ref<Mesh> Mesh, const std::vector<Ref<Texture2D>>& Materials);
        Ref<Mesh> mesh;
        std::vector<Ref<Texture2D>> materials;

        static const AssetType GetType() { return AssetType::Model; }
        virtual const AssetType GetAssetType() const override { return AssetType::Model; }
    };
}