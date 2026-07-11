#pragma once

#include "Aether/Audio/AudioAPI.h"

namespace Aether {

    class AETHER_API AudioSystem
    {
    public:
        void Init();
        void Shutdown();

        Handle<AudioSource> CreateSource(const std::string& path, AudioType type);
        void DestroySource(Handle<AudioSource> handle);
        bool IsActive(Handle<AudioSource> handle);

        void Play(Handle<AudioSource> handle);
        void Pause(Handle<AudioSource> handle);
        void Stop(Handle<AudioSource> handle);

        void SetGlobalVolume(float value);
        void SetMaxActiveSource(uint32_t value);

        void SetVolume(Handle<AudioSource> handle, float value);
        void SetPan(Handle<AudioSource> handle, float value);
        void SetLooping(Handle<AudioSource> handle, bool value);
        void SetPlaybackSpeed(Handle<AudioSource> handle, float value);
        void Seek(Handle<AudioSource> handle, float value);

        // 3D only
        void SetSpeedSound(float value);
        void SetPosition(Handle<AudioSource> handle, const glm::vec3& position);
        void SetVelocity(Handle<AudioSource> handle, const glm::vec3& velocity);
        void SetDistance(Handle<AudioSource> handle, float minDist, float maxDist);
        void SetAttenuation(Handle<AudioSource> handle, AudioAttenuation attenuation);

        void UpdateListener(const AudioListener& listener);

    private:
        Scope<AudioAPI> s_AudioAPI;
    };
}