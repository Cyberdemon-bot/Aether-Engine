#include "aepch.h"
#include "Aether/Animation/AnimationClip.h"

namespace Aether {
    void ClipLibrary::Init()
    {
        GetClips().reserve(128);
        AE_CORE_INFO("ClipLibrary initialized!");
    }

    void ClipLibrary::Shutdown()
    {
        GetClips().clear();
    }

    void ClipLibrary::Add(Ref<Clip> obj, UUID id)
    {
        auto& clips = GetClips();
        if (clips.find(id) != clips.end())
        {
            AE_CORE_ERROR("Clip Library: ID already exists");
            return;
        }

        if (!obj)
        {
            AE_CORE_ERROR("Clip Library: Cannot add null obj");
            return;
        }
        clips[id] = obj;
    }

    Ref<Clip> ClipLibrary::Get(UUID id)
    {
        auto& clips = GetClips();
        if (clips.find(id) != clips.end())
            return clips[id];
        
        AE_CORE_ERROR("Clip Library: ID does not exist yet!");
        return nullptr;
    }

    bool ClipLibrary::Exists(UUID id)
    {
        auto& clips = GetClips();
        return clips.find(id) != clips.end();
    }

    std::unordered_map<UUID, Ref<Clip>>& ClipLibrary::GetClips()
    {
        static std::unordered_map<UUID, Ref<Clip>> s_Clips;
        return s_Clips;
    }
}
