#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Container/ResourcePool.h"
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

        Handle<SkeletonTag> GetHandle() { return handle; }

        template<typename... Args>
        static Ref<Skeleton> Create(Args&&... args)
        {
            return CreateRef<Skeleton>(std::forward<Args>(args)...);
        }
    private:
        Handle<SkeletonTag> handle;

        static Scope<Skeleton> CreateImpl(const SkeletonSpec& spec) { return CreateScope<Skeleton>(spec);}
        friend class AssetManager;
    };
}