#pragma once

#include "Aether/Renderer/Texture.h"
#include "Aether/Assets/AssetManager.h"

namespace Aether {
    class Image : public Asset
    {
    public: 
        
    private:
        Ref<Texture2D> m_Texture;
        static const AssetType GetType() { return AssetType::Image; }
        virtual const AssetType GetAssetType() const override { return AssetType::Image; }
        friend class AssetManager;
    }
}
