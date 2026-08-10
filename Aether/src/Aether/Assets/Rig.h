#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"

namespace Aether {

    struct RSkeleton;
    struct RClip;

    struct Skeleton : public Asset
    {
        Skeleton(Handle<RSkeleton> handle, uint32_t count)
        {
            m_Handle = handle;
            m_JointCount = count;
        }
        Handle<RSkeleton> m_Handle;
        uint32_t m_JointCount;
    };

    struct Clip : public Asset
    {
        Clip(Handle<RClip> handle, float duration)
        {
            m_Handle = handle;
            m_Duration = duration;
        }
        Handle<RClip> m_Handle;
        float m_Duration;
    };
}