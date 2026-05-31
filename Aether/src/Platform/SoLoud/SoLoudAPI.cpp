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

        m_Pool.Init();
    }

    void SoLoudAPI::Shutdown()
    {
        if (!m_Initialized) return;
        soloud.stopAll();
        soloud.deinit();
        m_Pool.Clear();
    }

    Handle<AudioSource> SoLoudAPI::CreateSource(const std::string& path, AudioType type)
    {
        if (!m_Initialized) return Handle<AudioSource>::MakeInvalid();
        Handle<AudioSource> handle = m_Pool.CreateResource();
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source)
        {
            AE_CORE_ERROR("SoLoudAPI::CreateSource: pool allocation failed");
            return Handle<AudioSource>::MakeInvalid();
        }

        SoLoud::result res = source->wav.load(path.c_str());
        if (res != SoLoud::SO_NO_ERROR)
        {
            AE_CORE_ERROR("SoLoudAPI::CreateSource: failed to load '{0}'", path);
            m_Pool.DestroyResource(handle);
            return Handle<AudioSource>::MakeInvalid();
        }

        source->type = type;
        return handle;
    }

    void SoLoudAPI::DestroySource(Handle<AudioSource> handle)
    {
        if (!m_Initialized) return;
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source) return;

        if (soloud.isValidVoiceHandle(source->voiceHandle)) 
        {
            soloud.setPause(source->voiceHandle, true); 
            soloud.stop(source->voiceHandle);
        }

        m_Pool.DestroyResource(handle);
    }

    bool SoLoudAPI::IsActive(Handle<AudioSource> handle)
    {
        if (!m_Initialized) return false;
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source) return false;
        return soloud.isValidVoiceHandle(source->voiceHandle);
    }

    void SoLoudAPI::Play(Handle<AudioSource> handle)
    {
        if (!m_Initialized) return;
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source) return;

        if (!soloud.isValidVoiceHandle(source->voiceHandle))
        {
            auto& s = source->state;
            if (source->type == AudioType::Audio2D) source->voiceHandle = soloud.play(source->wav);
            else source->voiceHandle = soloud.play3d(source->wav, s.position.x, s.position.y, s.position.z);

            int voice = source->voiceHandle;
            soloud.setVolume(voice, s.volume);
            soloud.setPan(voice, s.pan);
            soloud.setRelativePlaySpeed(voice, s.playback_speed);
            soloud.setLooping(voice, s.looping);

            if (source->type == AudioType::Audio3D)
            {
                auto& cfg = source->config;
                soloud.set3dSourceAttenuation(voice, ToSoLoudAttenuation(cfg.attenuation), 1.0f);
                soloud.set3dSourceMinMaxDistance(voice, cfg.minDistance, cfg.maxDistance);
                soloud.set3dSourceVelocity(voice, s.velocity.x, s.velocity.y, s.velocity.z);
                soloud.set3dSourcePosition(voice, s.position.x, s.position.y, s.position.z);
            }
        }

        source->state.pausing = false;
        soloud.setPause(source->voiceHandle, false);
    }

    void SoLoudAPI::Pause(Handle<AudioSource> handle)
    {
        if (!m_Initialized) return;
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source) return;

        source->state.pausing = true;
        if (soloud.isValidVoiceHandle(source->voiceHandle))
            soloud.setPause(source->voiceHandle, true);
    }

    void SoLoudAPI::Stop(Handle<AudioSource> handle)
    {
        if (!m_Initialized) return;
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source) return;

        if (soloud.isValidVoiceHandle(source->voiceHandle))
            soloud.stop(source->voiceHandle);
    }

    void SoLoudAPI::SetVolume(Handle<AudioSource> handle, float value)
    {
        if (!m_Initialized) return;
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source) return;

        source->state.volume = value;
        if (soloud.isValidVoiceHandle(source->voiceHandle))
            soloud.setVolume(source->voiceHandle, value);
    }

    void SoLoudAPI::SetPan(Handle<AudioSource> handle, float value)
    {
        if (!m_Initialized) return;
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source) return;

        source->state.pan = value;
        if (soloud.isValidVoiceHandle(source->voiceHandle))
            soloud.setPan(source->voiceHandle, value);
    }

    void SoLoudAPI::SetLooping(Handle<AudioSource> handle, bool value)
    {
        if (!m_Initialized) return;
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source) return;

        source->state.looping = value;
        if (soloud.isValidVoiceHandle(source->voiceHandle))
            soloud.setLooping(source->voiceHandle, value);
    }

    void SoLoudAPI::SetPlaybackSpeed(Handle<AudioSource> handle, float value)
    {
        if (!m_Initialized) return;
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source) return;

        source->state.playback_speed = value;
        if (soloud.isValidVoiceHandle(source->voiceHandle))
            soloud.setRelativePlaySpeed(source->voiceHandle, value);
    }

    void SoLoudAPI::SetPosition(Handle<AudioSource> handle, const glm::vec3& position)
    {
        if (!m_Initialized) return;
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source || source->type != AudioType::Audio3D) return;

        source->state.position = position;
        if (soloud.isValidVoiceHandle(source->voiceHandle))
            soloud.set3dSourcePosition(source->voiceHandle, position.x, position.y, position.z);
    }

    void SoLoudAPI::SetVelocity(Handle<AudioSource> handle, const glm::vec3& velocity)
    {
        if (!m_Initialized) return;
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source || source->type != AudioType::Audio3D) return;

        source->state.velocity = velocity;
        if (soloud.isValidVoiceHandle(source->voiceHandle))
            soloud.set3dSourceVelocity(source->voiceHandle, velocity.x, velocity.y, velocity.z);
    }

    void SoLoudAPI::SetDistance(Handle<AudioSource> handle, float minDist, float maxDist)
    {
        if (!m_Initialized) return;
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source || source->type != AudioType::Audio3D) return;

        source->config.minDistance = minDist;
        source->config.maxDistance = maxDist;
        if (soloud.isValidVoiceHandle(source->voiceHandle))
            soloud.set3dSourceMinMaxDistance(source->voiceHandle, minDist, maxDist);
    }

    void SoLoudAPI::SetAttenuation(Handle<AudioSource> handle, AudioAttenuation attenuation)
    {
        if (!m_Initialized) return;
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source || source->type != AudioType::Audio3D) return;

        source->config.attenuation = attenuation;
        if (soloud.isValidVoiceHandle(source->voiceHandle))
            soloud.set3dSourceAttenuation(source->voiceHandle, ToSoLoudAttenuation(attenuation), 1.0f);
    }

    void SoLoudAPI::Seek(Handle<AudioSource> handle, float value)
    {
        if (!m_Initialized) return;
        AudioSource* source = m_Pool.GetResource(handle);
        if (!source) return;
        if (soloud.isValidVoiceHandle(source->voiceHandle))
            soloud.seek(source->voiceHandle, value);
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
}