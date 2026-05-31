#pragma once

#include "Aether/Audio/AudioAPI.h"

namespace Aether {

    class AETHER_API AudioSystem
    {
    public:
        static void Init() { s_AudioAPI->Init(); }
        static void Shutdown() { s_AudioAPI->Shutdown(); }

        static Handle<AudioSource> CreateSource(const std::string& path, AudioType type) { return s_AudioAPI->CreateSource(path, type); }
        static void DestroySource(Handle<AudioSource> handle) { s_AudioAPI->DestroySource(handle); }
        static bool IsActive(Handle<AudioSource> handle) { return s_AudioAPI->IsActive(handle); }

        static void Play(Handle<AudioSource> handle) { s_AudioAPI->Play(handle); }
        static void Pause(Handle<AudioSource> handle) { s_AudioAPI->Pause(handle); }
        static void Stop(Handle<AudioSource> handle) { s_AudioAPI->Stop(handle); }

        static void SetGlobalVolume(float value) { s_AudioAPI->SetGlobalVolume(value); }
        static void SetMaxActiveSource(uint32_t value) { s_AudioAPI->SetMaxActiveSource(value); }

        static void SetVolume(Handle<AudioSource> handle, float value) { s_AudioAPI->SetVolume(handle, value); }
        static void SetPan(Handle<AudioSource> handle, float value) { s_AudioAPI->SetPan(handle, value); }
        static void SetLooping(Handle<AudioSource> handle, bool value) { s_AudioAPI->SetLooping(handle, value); }
        static void SetPlaybackSpeed(Handle<AudioSource> handle, float value) { s_AudioAPI->SetPlaybackSpeed(handle, value); }
        static void Seek(Handle<AudioSource> handle, float value) { s_AudioAPI->Seek(handle, value); }

        // 3D only
        static void SetSpeedSound(float value) { s_AudioAPI->SetSpeedSound(value); }
        static void SetPosition(Handle<AudioSource> handle, const glm::vec3& position) { s_AudioAPI->SetPosition(handle, position); }
        static void SetVelocity(Handle<AudioSource> handle, const glm::vec3& velocity) { s_AudioAPI->SetVelocity(handle, velocity); }
        static void SetDistance(Handle<AudioSource> handle, float minDist, float maxDist) { s_AudioAPI->SetDistance(handle, minDist, maxDist); }
        static void SetAttenuation(Handle<AudioSource> handle, AudioAttenuation attenuation) { s_AudioAPI->SetAttenuation(handle, attenuation); }

        static void UpdateListener(const AudioListener& listener) { s_AudioAPI->UpdateListener(listener); }

    private:
        static Scope<AudioAPI> s_AudioAPI;
    };
}