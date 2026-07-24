#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"

namespace Aether {

    struct RSkeleton;
    struct RClip;

    struct Skeleton : public Asset
    {
        Skeleton() = default;
        Handle<RSkeleton> m_Handle;
        uint32_t m_JointCount;
    };

    struct Clip : public Asset
    {
        Clip() = default;
        
        Handle<RClip> m_Handle;
        float m_Duration;
    };
}