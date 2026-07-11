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

    Handle<AudioSource> AudioSystem::CreateSource(const std::string& path, AudioType type) 
    { 
        return s_AudioAPI->CreateSource(path, type); 
    }

    void AudioSystem::DestroySource(Handle<AudioSource> handle) 
    { 
        s_AudioAPI->DestroySource(handle); 
    }

    bool AudioSystem::IsActive(Handle<AudioSource> handle)
    { 
        return s_AudioAPI->IsActive(handle); 
    }

    void AudioSystem::Play(Handle<AudioSource> handle) 
    { 
        s_AudioAPI->Play(handle); 
    }

    void AudioSystem::Pause(Handle<AudioSource> handle) 
    { 
        s_AudioAPI->Pause(handle); 
    }

    void AudioSystem::Stop(Handle<AudioSource> handle) 
    { 
        s_AudioAPI->Stop(handle); 
    }

    void AudioSystem::SetGlobalVolume(float value) 
    { 
        s_AudioAPI->SetGlobalVolume(value); 
    }

    void AudioSystem::SetMaxActiveSource(uint32_t value) 
    { 
        s_AudioAPI->SetMaxActiveSource(value); 
    }

    void AudioSystem::SetVolume(Handle<AudioSource> handle, float value) 
    { 
        s_AudioAPI->SetVolume(handle, value); 
    }

    void AudioSystem::SetPan(Handle<AudioSource> handle, float value) 
    { 
        s_AudioAPI->SetPan(handle, value); 
    }

    void AudioSystem::SetLooping(Handle<AudioSource> handle, bool value) 
    { 
        s_AudioAPI->SetLooping(handle, value); 
    }

    void AudioSystem::SetPlaybackSpeed(Handle<AudioSource> handle, float value)
    { 
        s_AudioAPI->SetPlaybackSpeed(handle, value); 
    }

    void AudioSystem::Seek(Handle<AudioSource> handle, float value) 
    { 
        s_AudioAPI->Seek(handle, value); 
    }

    // 3D only
    void AudioSystem::SetSpeedSound(float value) 
    { 
        s_AudioAPI->SetSpeedSound(value); 
    }

    void AudioSystem::SetPosition(Handle<AudioSource> handle, const glm::vec3& position) 
    { 
        s_AudioAPI->SetPosition(handle, position); 
    }

    void AudioSystem::SetVelocity(Handle<AudioSource> handle, const glm::vec3& velocity) 
    { 
        s_AudioAPI->SetVelocity(handle, velocity); 
    }

    void AudioSystem::SetDistance(Handle<AudioSource> handle, float minDist, float maxDist) 
    { 
        s_AudioAPI->SetDistance(handle, minDist, maxDist); 
    }

    void AudioSystem::SetAttenuation(Handle<AudioSource> handle, AudioAttenuation attenuation) 
    { 
        s_AudioAPI->SetAttenuation(handle, attenuation);
    }

    void AudioSystem::UpdateListener(const AudioListener& listener) 
    { 
        s_AudioAPI->UpdateListener(listener); 
    }
}