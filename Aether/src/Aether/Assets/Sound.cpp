#include "aepch.h"
#include "Aether/Assets/Sound.h"
#include "Platform/SoLoud/SoLoudSound.h"
#include "Aether/Audio/AudioAPI.h"

namespace Aether {

    Scope<Sound> Sound::CreateImpl(const std::string& path)
    {
        switch (AudioAPI::GetAPI())
        {
            case AudioAPI::API::None:   AE_CORE_ASSERT(false, "AudioAPI::None is currently not supported!"); return nullptr;
            case AudioAPI::API::SoLoud: return CreateScope<SoLoudSound>(path);
        }

        AE_CORE_ASSERT(false, "Unknown AudioAPI!");
        return nullptr;
    }
}