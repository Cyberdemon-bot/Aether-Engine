#include "aepch.h"
#include "Platform/SoLoud/SoLoudAPI.h"

namespace Aether {

    SoLoud::AudioSource::ATTENUATION_MODELS SoLoudAPI::ToSoLoudAttenuation(AudioAttenuation attenuation)
    {
        switch (attenuation)
        {
            case AudioAttenuation::NO_ATTENUATION: return SoLoud::AudioSource::NO_ATTENUATION;
            case AudioAttenuation::LINEAR_DISTANCE: return SoLoud::AudioSource::LINEAR_DISTANCE;
            case AudioAttenuation::INVERSE_DISTANCE: return SoLoud::AudioSource::INVERSE_DISTANCE;
            case AudioAttenuation::EXPONENTIAL_DISTANCE: return SoLoud::AudioSource::EXPONENTIAL_DISTANCE;
        }
        return SoLoud::AudioSource::NO_ATTENUATION;
    }

    void SoLoudAPI::Init()
    {
        auto res = soloud.init();
        m_Initialized = (res == SoLoud::SO_NO_ERROR);

        if (!m_Initialized)
        {
            AE_CORE_ERROR("SoLoud init failed: {}", soloud.getErrorString(res));
            return;
        }

        m_Sources.Init();
        m_Players.Init();
    }

    void SoLoudAPI::Shutdown()
    {
        if (!m_Initialized) return;
        soloud.stopAll();
        soloud.deinit();
        m_Sources.Shutdown();
        m_Players.Shutdown();
    }

    Handle<AudioSource> SoLoudAPI::CreateSource(const std::string& path)
    {
        if (!m_Initialized) return Handle<AudioSource>::MakeInvalid();
        Handle<AudioSource> handle = m_Sources.CreateResource();
        auto* source = m_Sources.GetResource(handle);

        SoLoud::result res = source->wav.load(path.c_str());
        if (res != SoLoud::SO_NO_ERROR)
        {
            AE_CORE_ERROR("Failed to load '{0}'", path);
            m_Sources.DestroyResource(handle);
            return Handle<AudioSource>::MakeInvalid();
        }
        return handle;
    }

    void SoLoudAPI::DestroySource(Handle<AudioSource> handle)
    {
        if (!m_Initialized) return;
        auto* source = m_Sources.GetResource(handle);
        if (!source) return;

        soloud.stopAudioSource(source->wav);
        m_Sources.DestroyResource(handle);
    }

    Handle<AudioPlayer> SoLoudAPI::CreatePlayer(Handle<AudioSource> source, AudioType type)
    {
        if (!m_Initialized) return Handle<AudioPlayer>::MakeInvalid();
        auto* s = m_Sources.GetResource(source);
        if (!s) return Handle<AudioPlayer>::MakeInvalid();

        auto handle = m_Players.CreateResource();
        auto* player = m_Players.GetResource(handle);
        player->source = source;
        player->type = type;

        return handle;
    }

    void SoLoudAPI::DestroyPlayer(Handle<AudioPlayer> handle)
    {
        m_Players.DestroyResource(handle);
    }

    void SoLoudAPI::Play(Handle<AudioPlayer> handle)
    {
        auto* player = GetPlayerAndValidate(handle);
        if (!player) return;

        if (!soloud.isValidVoiceHandle(player->voice))
        {
            auto& s = player->state;
            auto& _3d = s._3dinfo;
            auto* source = m_Sources.GetResource(player->source);
            if (!source) return;
            if (player->type == AudioType::Audio2D) player->voice = soloud.play(source->wav);
            else player->voice = soloud.play3d(source->wav, _3d.position.x, _3d.position.y, _3d.position.z);

            int voice = player->voice;
            soloud.setVolume(voice, s.volume);
            soloud.setPan(voice, s.pan);
            soloud.setRelativePlaySpeed(voice, s.playback_speed);
            soloud.setLooping(voice, s.looping);

            if (player->type == AudioType::Audio3D)
            {
                soloud.set3dSourceAttenuation(voice, ToSoLoudAttenuation(_3d.attenuation), 1.0f);
                soloud.set3dSourceMinMaxDistance(voice, _3d.minDistance, _3d.maxDistance);
                soloud.set3dSourceVelocity(voice, _3d.velocity.x, _3d.velocity.y, _3d.velocity.z);
                soloud.set3dSourcePosition(voice, _3d.position.x, _3d.position.y, _3d.position.z);
            }
        }

        player->state.pausing = false;
        soloud.setPause(player->voice, false);
    }

    void SoLoudAPI::Pause(Handle<AudioPlayer> handle)
    {
        auto* player = GetPlayerAndValidate(handle);
        if (!player) return;

        player->state.pausing = true;
        if (soloud.isValidVoiceHandle(player->voice))
            soloud.setPause(player->voice, true);
    }

    void SoLoudAPI::Stop(Handle<AudioPlayer> handle)
    {
        auto* player = GetPlayerAndValidate(handle);
        if (!player) return;

        if (soloud.isValidVoiceHandle(player->voice))
            soloud.stop(player->voice);
    }

    void SoLoudAPI::Seek(Handle<AudioPlayer> handle, float value)
    {
        auto* player = GetPlayerAndValidate(handle);
        if (!player) return;
        if (soloud.isValidVoiceHandle(player->voice))
            soloud.seek(player->voice, value);
    }

    void SoLoudAPI::Update()
    {
        if (!m_Initialized) return;
        m_Players.Loop([this](const SoloudPlayer& player)
        {
            int voice = player.voice;
            auto& state = player.state;
            auto& _3d = state._3dinfo;
            if (!soloud.isValidVoiceHandle(player.voice)) return;

            soloud.setVolume(voice, state.volume);
            soloud.setPan(voice, state.pan);
            soloud.setLooping(voice, state.looping);
            soloud.setRelativePlaySpeed(voice, state.playback_speed);
            soloud.set3dSourcePosition(voice, _3d.position.x, _3d.position.y, _3d.position.z);
            soloud.set3dSourceVelocity(voice, _3d.velocity.x, _3d.velocity.y, _3d.velocity.z);
            soloud.set3dSourceMinMaxDistance(voice, _3d.minDistance, _3d.minDistance);
            soloud.set3dSourceAttenuation(voice, ToSoLoudAttenuation(_3d.attenuation), 1.0f);
        });
    }

    AudioState* SoLoudAPI::GetState(Handle<AudioPlayer> player)
    {
        auto* p = GetPlayerAndValidate(player);
        if (!p) return nullptr;
        return &p->state;
    }

    void SoLoudAPI::UpdateListener(const AudioListener& listener)
    {
        if (!m_Initialized) return;
        soloud.set3dListenerPosition(listener.position.x, listener.position.y, listener.position.z);
        soloud.set3dListenerVelocity(listener.velocity.x, listener.velocity.y, listener.velocity.z);
        soloud.set3dListenerAt(listener.forward.x, listener.forward.y, listener.forward.z);
        soloud.set3dListenerUp(listener.up.x, listener.up.y, listener.up.z);
        soloud.update3dAudio();
    }

    SoloudPlayer* SoLoudAPI::GetPlayerAndValidate(Handle<AudioPlayer> handle)
    {
        if (!m_Initialized) return nullptr;
        auto* player = m_Players.GetResource(handle);
        if (!player) return nullptr;

        auto* source = m_Sources.GetResource(player->source);
        if (!source)
        {
            m_Players.DestroyResource(handle);
            return nullptr;
        }

        return player;
    }
}