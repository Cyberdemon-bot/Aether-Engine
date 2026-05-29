#pragma once

#include "Aether/Audio/AudioAPI.h"
#include "Aether/Container/ResourcePool.h"
#include "soloud.h"
#include "soloud_wav.h"

namespace Aether {

    struct AudioSource
    {
        SoLoud::Wav wav; 
        AudioType type = AudioType::Audio2D;
        Audio3DConfig config;
        AudioState state;
        int voiceHandle = 0; 

        AudioSource() = default;
        AudioSource(const AudioSource&)            = delete;
        AudioSource& operator=(const AudioSource&) = delete;
        AudioSource(AudioSource&&) noexcept        = default;
        AudioSource& operator=(AudioSource&&) noexcept = default;
    };

    class SoLoudAPI : public AudioAPI
    {
    public:
        virtual void Init() override;
        virtual void Shutdown() override;
        virtual void Update() override; 

        virtual Handle<AudioSource> CreateSource(const std::string& path, AudioType type) override;
        virtual void DestroySource(Handle<AudioSource> handle) override;
        virtual bool IsActive(Handle<AudioSource> handle) override;

        virtual void Play(Handle<AudioSource> handle) override;
        virtual void Pause(Handle<AudioSource> handle) override;
        virtual void Stop(Handle<AudioSource> handle) override;

        virtual void SetGlobalVolume(float value) override { soloud.setGlobalVolume(value); }
        virtual void SetMaxActiveSource(uint32_t value) override { soloud.setMaxActiveVoiceCount(value); }

        virtual void SetVolume(Handle<AudioSource> handle, float value) override;
        virtual void SetPan(Handle<AudioSource> handle, float value) override;
        virtual void SetLooping(Handle<AudioSource> handle, bool value) override;
        virtual void SetPlaybackSpeed(Handle<AudioSource> handle, float value) override;
        virtual void Seek(Handle<AudioSource> handle, float value) override;

        // 3D only
        virtual void SetSpeedSound(float value) override { soloud.set3dSoundSpeed(value); }
        virtual void SetPosition(Handle<AudioSource> handle, const glm::vec3& position) override;
        virtual void SetVelocity(Handle<AudioSource> handle, const glm::vec3& velocity) override;
        virtual void SetDistance(Handle<AudioSource> handle, float minDist, float maxDist) override;
        virtual void SetAttenuation(Handle<AudioSource> handle, AudioAttenuation attenuation) override;

        virtual void UpdateListener(const AudioListener& listener) override;

    private:
        void UpdateSource(AudioSource& source); 
        static SoLoud::AudioSource::ATTENUATION_MODELS ToSoLoudAttenuation(AudioAttenuation attenuation);
        SoLoud::Soloud soloud;
        bool m_Initialized = false;
        ResourcePool<Handle<AudioSource>, AudioSource> m_Pool;
    };
}