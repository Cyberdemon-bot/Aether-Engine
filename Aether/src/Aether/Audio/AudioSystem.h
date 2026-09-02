#pragma once

#include "Aether/Audio/AudioAPI.h"

namespace Aether {

    class AETHER_API AudioSystem
    {
    public:
        void Init();
        void Shutdown();
        void OnUpdate();
        
        Handle<AudioSource> CreateSource(const std::string& path);
        Handle<AudioSource> CreateSource(const uint8_t* data, size_t size);
        void DestroySource(Handle<AudioSource> handle);
       
        Handle<AudioPlayer> CreatePlayer(Handle<AudioSource> source, AudioType type);
        void DestroyPlayer(Handle<AudioPlayer> handle);

        void Play(Handle<AudioPlayer> handle);
        void Pause(Handle<AudioPlayer> handle);
        void Stop(Handle<AudioPlayer> handle);
        void Seek(Handle<AudioPlayer> handle, float value);

        bool Modify(Handle<AudioPlayer> handle, Delegate<void(PlayerEditProxy&)> modifier);
        void UpdateListener(const AudioListener& listener);
    private:
        Scope<AudioAPI> s_AudioAPI;
    };
}