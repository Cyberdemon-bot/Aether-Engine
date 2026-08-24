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
        m_DestroyQueue.reserve(32);
    }

    void SoLoudAPI::Shutdown()
    {
        if (!m_Initialized) return;
        soloud.stopAll();
        soloud.deinit();
        m_Sources.Shutdown();
        m_Players.Shutdown();
        m_DestroyQueue.clear(); m_DestroyQueue.shrink_to_fit();
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
        player->self_handle = handle;
        player->source = source;
        player->type = type;

        return handle;
    }

    void SoLoudAPI::DestroyPlayer(Handle<AudioPlayer> handle)
    {
        if (!m_Initialized) return;
        auto* player = m_Players.GetResource(handle);
        if (!player) return;

        soloud.stop(player->voice);
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
            bool startPaused = player->state.pausing;
            if (player->type == AudioType::Audio2D) player->voice = soloud.play(source->wav, s.volume, s.pan, startPaused);
            else player->voice = soloud.play3d(source->wav, _3d.position.x, _3d.position.y, _3d.position.z, 
                                            _3d.velocity.x, _3d.velocity.y, _3d.velocity.z, 
                                            s.volume, startPaused);

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

    void SoLoudAPI::OnUpdate()
    {
        if (!m_Initialized) return;
        bool has3DChanged = false;
        m_Players.Loop([this, &has3DChanged](SoloudPlayer& player)
        {
            if (!m_Sources.GetResource(player.source)) 
            {
                m_DestroyQueue.push_back(player.self_handle);
                return;
            }
            if (player.dirty_flags == AudioDirtyFlags::DIRTY_NONE) return;
            if (!soloud.isValidVoiceHandle(player.voice)) return;

            int voice = player.voice;
            auto& state = player.state;
            auto& _3d = state._3dinfo;
            uint16_t f = player.dirty_flags;

            if (f & AudioDirtyFlags::DIRTY_VOLUME) soloud.setVolume(voice, state.volume);
            if (f & AudioDirtyFlags::DIRTY_PAN) soloud.setPan(voice, state.pan);
            if (f & AudioDirtyFlags::DIRTY_SPEED) soloud.setRelativePlaySpeed(voice, state.playback_speed);
            if (f & AudioDirtyFlags::DIRTY_LOOP) soloud.setLooping(voice, state.looping);
            if (f & AudioDirtyFlags::DIRTY_PAUSE) soloud.setPause(voice, state.pausing);
            if (player.type == AudioType::Audio3D)
            {   
                if (f & AudioDirtyFlags::DIRTY_3D_POS) soloud.set3dSourcePosition(voice, _3d.position.x, _3d.position.y, _3d.position.z);
                if (f & AudioDirtyFlags::DIRTY_3D_VEL) soloud.set3dSourceVelocity(voice, _3d.velocity.x, _3d.velocity.y, _3d.velocity.z);
                if (f & AudioDirtyFlags::DIRTY_3D_DIST) soloud.set3dSourceMinMaxDistance(voice, _3d.minDistance, _3d.maxDistance);
                if (f & AudioDirtyFlags::DIRTY_3D_ATT) soloud.set3dSourceAttenuation(voice, ToSoLoudAttenuation(_3d.attenuation), 1.0f);
                if (f & (AudioDirtyFlags::DIRTY_3D_POS | AudioDirtyFlags::DIRTY_3D_VEL)) has3DChanged = true;
            }

            player.dirty_flags = AudioDirtyFlags::DIRTY_NONE;
        });

        if (has3DChanged) soloud.update3dAudio();
        for (auto& handle : m_DestroyQueue) DestroyPlayer(handle);
        m_DestroyQueue.clear();
    }

    bool SoLoudAPI::Modify(Handle<AudioPlayer> handle, Delegate<void(PlayerEditProxy&)> modifier)
    {
        auto* p = GetPlayerAndValidate(handle);
        if (!p) return false;

        PlayerEditProxy proxy(p->state);
        modifier(proxy);

        if (proxy.dirty_flags != AudioDirtyFlags::DIRTY_NONE) 
        {
            p->state = proxy.temp;
            p->dirty_flags |= proxy.dirty_flags;
        }

        return true;
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

        return player;
    }
}