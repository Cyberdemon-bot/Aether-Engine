#include "Platform/SoLoud/SoLoudAPI.h"
#include "Aether/Core/Assert.h"

namespace Aether {
    AudioAPI::API AudioAPI::s_API = AudioAPI::API::SoLoud;

    Scope<AudioAPI> AudioAPI::Create()
    {
        switch (s_API)
		{
			case AudioAPI::API::None:    AE_CORE_ASSERT(false, "AudioAPI::None is currently not supported!"); return nullptr;
			case AudioAPI::API::SoLoud:  return CreateScope<SoLoudAPI>();
		}

		AE_CORE_ASSERT(false, "Unknown AudioAPI!");
		return nullptr;
    }
}