#include "aepch.h"
#include "Aether/Audio/AudioSystem.h"

namespace Aether {

    void AudioSystem::Init() 
    { 
        s_AudioAPI = AudioAPI::Create();
        s_AudioAPI->Init(); 
    }

    void AudioSystem::Shutdown() 
    { 
        s_AudioAPI->Shutdown(); 
    }
}