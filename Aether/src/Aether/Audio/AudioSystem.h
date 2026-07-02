#pragma once

#include "Aether/Audio/AudioAPI.h"

namespace Aether {

    class AETHER_API AudioSystem
    {
    public:
        void Init();
        void Shutdown();

        Handle<AudioSource> CreateSource(const std::string& path, AudioType type) { return s_AudioAPI->CreateSource(path, type); }
        void DestroySource(Handle<AudioSource> handle) { s_AudioAPI->DestroySource(handle); }
        bool IsActive(Handle<AudioSource> handle) { return s_AudioAPI->IsActive(handle); }

        void Play(Handle<AudioSource> handle) { s_AudioAPI->Play(handle); }
        void Pause(Handle<AudioSource> handle) { s_AudioAPI->Pause(handle); }
        void Stop(Handle<AudioSource> handle) { s_AudioAPI->Stop(handle); }

        void SetGlobalVolume(float value) { s_AudioAPI->SetGlobalVolume(value); }
        void SetMaxActiveSource(uint32_t value) { s_AudioAPI->SetMaxActiveSource(value); }

        void SetVolume(Handle<AudioSource> handle, float value) { s_AudioAPI->SetVolume(handle, value); }
        void SetPan(Handle<AudioSource> handle, float value) { s_AudioAPI->SetPan(handle, value); }
        void SetLooping(Handle<AudioSource> handle, bool value) { s_AudioAPI->SetLooping(handle, value); }
        void SetPlaybackSpeed(Handle<AudioSource> handle, float value) { s_AudioAPI->SetPlaybackSpeed(handle, value); }
        void Seek(Handle<AudioSource> handle, float value) { s_AudioAPI->Seek(handle, value); }

        // 3D only
        void SetSpeedSound(float value) { s_AudioAPI->SetSpeedSound(value); }
        void SetPosition(Handle<AudioSource> handle, const glm::vec3& position) { s_AudioAPI->SetPosition(handle, position); }
        void SetVelocity(Handle<AudioSource> handle, const glm::vec3& velocity) { s_AudioAPI->SetVelocity(handle, velocity); }
        void SetDistance(Handle<AudioSource> handle, float minDist, float maxDist) { s_AudioAPI->SetDistance(handle, minDist, maxDist); }
        void SetAttenuation(Handle<AudioSource> handle, AudioAttenuation attenuation) { s_AudioAPI->SetAttenuation(handle, attenuation); }

        void UpdateListener(const AudioListener& listener) { s_AudioAPI->UpdateListener(listener); }

    private:
        Scope<AudioAPI> s_AudioAPI;
    };
}