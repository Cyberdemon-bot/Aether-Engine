#pragma once

#include <vector>
#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Core/ServiceManager.h"

namespace Aether {

    struct Skeleton : public Asset
    {
        Skeleton(const SkeletonSpec& spec)
        {
            auto rigit = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
            m_Handle = rigit->CreateSkeleton(spec);
            m_JointCount = spec.Joints.size();
        }

        Handle<Skeleton> m_Handle;
        uint32_t m_JointCount;
    };

    struct Clip : public Asset
    {
        Clip(const ClipSpec& spec, Handle<Skeleton> skeleton)
        {
            auto rigit = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>();
            m_Handle = rigit->CreateClip(spec, skeleton);
            m_Duration = spec.Duration;
        }
        
        Handle<Clip> m_Handle;
        float m_Duration;
    };
}