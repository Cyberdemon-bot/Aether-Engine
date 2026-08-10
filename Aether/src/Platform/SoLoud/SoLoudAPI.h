#pragma once

#include "soloud.h"
#include "soloud_wav.h"
#include "Aether/Audio/AudioAPI.h"
#include "Aether/Container/ResourcePool.h"

namespace Aether {

    struct SoloudPlayer
    {
        Handle<AudioSource> source;
        AudioType type = AudioType::Audio2D;
        AudioState state;
        int voice = 0;
    };

    struct SoloudSource
    {
        SoLoud::Wav wav;
    };

    class SoLoudAPI : public AudioAPI
    {
    public:
        virtual void Init() override;
        virtual void Shutdown() override;
        virtual void Update() override;
        
        virtual Handle<AudioSource> CreateSource(const std::string& path) override;
        virtual void DestroySource(Handle<AudioSource> handle) override;
       
        virtual Handle<AudioPlayer> CreatePlayer(Handle<AudioSource> source, AudioType type) override;
        virtual void DestroyPlayer(Handle<AudioPlayer> handle) override;

        virtual void Play(Handle<AudioPlayer> handle) override;
        virtual void Pause(Handle<AudioPlayer> handle) override;
        virtual void Stop(Handle<AudioPlayer> handle) override;
        virtual void Seek(Handle<AudioPlayer> handle, float value) override;

        virtual AudioState* GetState(Handle<AudioPlayer> player) override;
        virtual void UpdateListener(const AudioListener& listener) override;

    private:
        SoLoud::AudioSource::ATTENUATION_MODELS ToSoLoudAttenuation(AudioAttenuation attenuation);
        SoloudPlayer* GetPlayerAndValidate(Handle<AudioPlayer> handle);

        SoLoud::Soloud soloud;
        bool m_Initialized = false;
        ResourcePool<Handle<AudioSource>, SoloudSource> m_Sources;
        ResourcePool<Handle<AudioPlayer>, SoloudPlayer> m_Players;
    };
}