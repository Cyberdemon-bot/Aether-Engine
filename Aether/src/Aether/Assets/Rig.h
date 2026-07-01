#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"

namespace Aether {

    class Skeleton : public Asset
    {
    public:
        Skeleton(const SkeletonSpec& spec)
        {
            handle = AnimationSystem::GetModule<RigModule>()->CreateSkeleton(spec);
        }
        virtual ~Skeleton() = default;

        Handle<Skeleton> GetHandle() { return handle; }

        template<typename... Args>
        static Ref<Skeleton> Create(Args&&... args)
        {
            return CreateRef<Skeleton>(std::forward<Args>(args)...);
        }
    private:
        Handle<Skeleton> handle;
    };

    class Clip : public Asset
    {
    public:
        Clip(const ClipSpec& spec, Handle<Skeleton> skeleton)
        {
            handle = AnimationSystem::GetModule<RigModule>()->CreateClip(spec, skeleton);
        }
        virtual ~Clip() = default;

        Handle<Clip> GetHandle() { return handle; }

        template<typename... Args>
        static Ref<Clip> Create(Args&&... args)
        {
            return CreateRef<Clip>(std::forward<Args>(args)...);
        }
    private:
        Handle<Clip> handle;
    };
}