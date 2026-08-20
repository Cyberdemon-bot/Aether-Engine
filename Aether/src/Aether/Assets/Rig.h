#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"

namespace Aether {

    struct Skeleton;
    struct Clip;

    struct ASkeleton : public Asset
    {
        ASkeleton() = default;

        Handle<Skeleton> m_Handle;
        uint32_t m_JointCount;
    };

    struct AClip : public Asset
    {
        AClip() = default;

        Handle<Clip> m_Handle;
        float m_Duration;
    };
}