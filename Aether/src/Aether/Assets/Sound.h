#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Assets/AssetManager.h"
#include <string>

namespace Aether {

    class AETHER_API Sound : public Asset
    {
    public:
        virtual ~Sound() = default;

        virtual void* GetNativeHandle() = 0;

    private:
        static Scope<Sound> CreateImpl(const std::string& path);

        static const AssetType GetType() { return AssetType::Sound; }
        virtual const AssetType GetAssetType() const override { return AssetType::Sound; }
        friend class AssetManager;
    };
}