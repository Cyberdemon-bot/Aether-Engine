#pragma once

#include <vector>
#include "soloud.h"
#include "soloud_wav.h"
#include "Aether/Audio/AudioAPI.h"
#include "Aether/Container/ResourcePool.h"

namespace Aether {

    struct SoloudPlayer
    {
        Handle<AudioSource> source;
        Handle<AudioPlayer> self_handle;
        AudioType type = AudioType::Audio2D;
        AudioState state;
        uint16_t dirty_flags = AudioDirtyFlags::DIRTY_NONE;
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
        virtual void OnUpdate() override;
        
        virtual Handle<AudioSource> CreateSource(const std::string& path) override;
        virtual void DestroySource(Handle<AudioSource> handle) override;
       
        virtual Handle<AudioPlayer> CreatePlayer(Handle<AudioSource> source, AudioType type) override;
        virtual void DestroyPlayer(Handle<AudioPlayer> handle) override;

        virtual void Play(Handle<AudioPlayer> handle) override;
        virtual void Pause(Handle<AudioPlayer> handle) override;
        virtual void Stop(Handle<AudioPlayer> handle) override;
        virtual void Seek(Handle<AudioPlayer> handle, float value) override;

        virtual bool Modify(Handle<AudioPlayer> handle, Delegate<void(PlayerEditProxy&)> modifier) override;
        virtual void UpdateListener(const AudioListener& listener) override;

    private:
        SoLoud::AudioSource::ATTENUATION_MODELS ToSoLoudAttenuation(AudioAttenuation attenuation);
        SoloudPlayer* GetPlayerAndValidate(Handle<AudioPlayer> handle);

        SoLoud::Soloud soloud;
        bool m_Initialized = false;
        ResourcePool<Handle<AudioSource>, SoloudSource> m_Sources;
        ResourcePool<Handle<AudioPlayer>, SoloudPlayer> m_Players;

        std::vector<Handle<AudioPlayer>> m_DestroyQueue;
    };
}