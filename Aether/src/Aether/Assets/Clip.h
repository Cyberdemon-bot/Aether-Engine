#pragma once

#include "Aether/Assets/Asset.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Container/ResourcePool.h"
#include "Aether/Animation/AnimationSystem.h"
#include "Aether/Animation/RigModule.h"

namespace Aether {

    class Clip : public Asset
    {
    public:
        Clip(const ClipSpec& spec, Handle<SkeletonTag> skeleton)
        {
            handle = AnimationSystem::GetModule<RigModule>()->CreateClip(spec, skeleton);
        }
        virtual ~Clip() = default;

        Handle<ClipTag> GetHandle() { return handle; }

        template<typename... Args>
        static Ref<Clip> Create(Args&&... args)
        {
            return CreateRef<Clip>(std::forward<Args>(args)...);
        }
    private:
        Handle<ClipTag> handle;

        static Scope<Clip> CreateImpl(const ClipSpec& spec, Handle<SkeletonTag> skeleton) { return CreateScope<Clip>(spec, skeleton);}
        friend class AssetManager;
    };
}