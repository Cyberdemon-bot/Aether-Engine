#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"
#include "Aether/Core/ServiceManager.h"

namespace Aether {

    class Skeleton : public Asset
    {
    public:
        Skeleton(const SkeletonSpec& spec)
        {
            handle = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>()->CreateSkeleton(spec);
        }
        virtual ~Skeleton() = default;

        Handle<Skeleton> GetHandle() { return handle; }
    private:
        Handle<Skeleton> handle;
    };

    class Clip : public Asset
    {
    public:
        Clip(const ClipSpec& spec, Handle<Skeleton> skeleton)
        {
            handle = ServiceManager::GetService<AnimationSystem>()->GetModule<RigModule>()->CreateClip(spec, skeleton);
        }
        virtual ~Clip() = default;

        Handle<Clip> GetHandle() { return handle; }
    private:
        Handle<Clip> handle;
    };
}