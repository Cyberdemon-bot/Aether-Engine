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

	void AudioSystem::Update()
	{
		s_AudioAPI->Update();
	}

    Handle<AudioSource> AudioSystem::CreateSource(const std::string& path) 
    { 
        return s_AudioAPI->CreateSource(path); 
    }

    void AudioSystem::DestroySource(Handle<AudioSource> handle) 
    { 
        s_AudioAPI->DestroySource(handle); 
    }

    Handle<AudioPlayer> AudioSystem::CreatePlayer(Handle<AudioSource> source, AudioType type)
	{
		return s_AudioAPI->CreatePlayer(source, type);
	}

    void AudioSystem::DestroyPlayer(Handle<AudioPlayer> handle)
	{
		s_AudioAPI->DestroyPlayer(handle);
	}

    void AudioSystem::Play(Handle<AudioPlayer> handle) 
    { 
        s_AudioAPI->Play(handle); 
    }

    void AudioSystem::Pause(Handle<AudioPlayer> handle) 
    { 
        s_AudioAPI->Pause(handle); 
    }

    void AudioSystem::Stop(Handle<AudioPlayer> handle) 
    { 
        s_AudioAPI->Stop(handle); 
    }

    void AudioSystem::Seek(Handle<AudioPlayer> handle, float value) 
    { 
        s_AudioAPI->Seek(handle, value); 
    }

	bool AudioSystem::Modify(Handle<AudioPlayer> handle, Delegate<void(PlayerEditProxy&)> modifier)
	{
		return s_AudioAPI->Modify(handle, std::move(modifier));
	}

    void AudioSystem::UpdateListener(const AudioListener& listener) 
    { 
        s_AudioAPI->UpdateListener(listener); 
    }
}