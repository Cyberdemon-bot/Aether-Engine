#include "aepch.h"
#include "Platform/SoLoud/SoLoudAPI.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Assets/Sound.h"

namespace Aether {

    // ─────────────────────────────────────────────────────────────────────────
    // Helpers
    // ─────────────────────────────────────────────────────────────────────────

    SoLoud::Wav* SoLoudAPI::GetWav(UUID soundID)
    {
        auto* sound = AssetManager::GetAsset<Sound>(soundID);
        if (!sound) return nullptr;
        return static_cast<SoLoud::Wav*>(sound->GetNativeHandle());
    }

    SoLoud::AudioSource::ATTENUATION_MODELS SoLoudAPI::ToSoLoudAttenuation(AudioAttenuation attenuation)
    {
        switch (attenuation)
        {
            case AudioAttenuation::NO_ATTENUATION:       return SoLoud::AudioSource::NO_ATTENUATION;
            case AudioAttenuation::LINEAR_DISTANCE:      return SoLoud::AudioSource::LINEAR_DISTANCE;
            case AudioAttenuation::INVERSE_DISTANCE:     return SoLoud::AudioSource::INVERSE_DISTANCE;
            case AudioAttenuation::EXPONENTIAL_DISTANCE: return SoLoud::AudioSource::EXPONENTIAL_DISTANCE;
        }
        return SoLoud::AudioSource::NO_ATTENUATION;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Lifecycle
    // ─────────────────────────────────────────────────────────────────────────

    void SoLoudAPI::Init()
    {
        soloud.init();
        m_Sources.reserve(32);
    }

    void SoLoudAPI::Shutdown()
    {
        soloud.stopAll();
        m_Sources.clear();
        soloud.deinit();
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Sources
    // ─────────────────────────────────────────────────────────────────────────

    void SoLoudAPI::CreateSource(UUID sourceID, UUID soundID, AudioType type)
    {
        if (m_Sources.count(sourceID)) return;

        if (!AssetManager::GetAsset<Sound>(soundID))
        {
            AE_CORE_WARN("SoLoudAPI::CreateSource: sound {0} not loaded in AssetManager", uint64_t(soundID));
            return;
        }

        auto& source   = m_Sources[sourceID];
        source.soundID = soundID;
        source.type    = type;
    }

    void SoLoudAPI::DestroySource(UUID sourceID)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        if (soloud.isValidVoiceHandle(it->second.handle))
            soloud.stop(it->second.handle);
        m_Sources.erase(it);
    }

    bool SoLoudAPI::IsActive(UUID sourceID)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return false;
        return soloud.isValidVoiceHandle(it->second.handle);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // Playback
    // ─────────────────────────────────────────────────────────────────────────

    void SoLoudAPI::Play(UUID sourceID)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        auto* wav = GetWav(it->second.soundID);
        if (!wav) return;

        if (!soloud.isValidVoiceHandle(it->second.handle))
        {
            if (it->second.type == AudioType::Audio2D)
                it->second.handle = soloud.play(*wav);
            else
                it->second.handle = soloud.play3d(*wav, 0, 0, 0);
        }

        it->second.state.pausing = false;
        int handle = it->second.handle;
        soloud.setPause(handle, false);
        soloud.setVolume(handle, it->second.state.volume);
        soloud.setPan(handle, it->second.state.pan);
        soloud.setRelativePlaySpeed(handle, it->second.state.playback_speed);
        soloud.setLooping(handle, it->second.state.looping);

        if (it->second.type == AudioType::Audio3D)
        {
            const auto& pos = it->second.state.position;
            const auto& vel = it->second.state.velocity;
            const auto& cfg = it->second.config;

            soloud.set3dSourceAttenuation(handle, ToSoLoudAttenuation(cfg.attenuation), 1.0f);
            soloud.set3dSourceMinMaxDistance(handle, cfg.minDistance, cfg.maxDistance);
            soloud.set3dSourceVelocity(handle, vel.x, vel.y, vel.z);
            soloud.set3dSourcePosition(handle, pos.x, pos.y, pos.z);
        }
    }

    void SoLoudAPI::Pause(UUID sourceID)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        it->second.state.pausing = true;
        if (soloud.isValidVoiceHandle(it->second.handle))
            soloud.setPause(it->second.handle, true);
    }

    void SoLoudAPI::Stop(UUID sourceID)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        if (soloud.isValidVoiceHandle(it->second.handle))
            soloud.stop(it->second.handle);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // State setters
    // ─────────────────────────────────────────────────────────────────────────

    void SoLoudAPI::SetVolume(UUID sourceID, float value)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        it->second.state.volume = value;
        if (soloud.isValidVoiceHandle(it->second.handle))
            soloud.setVolume(it->second.handle, value);
    }

    void SoLoudAPI::SetPan(UUID sourceID, float value)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        it->second.state.pan = value;
        if (soloud.isValidVoiceHandle(it->second.handle))
            soloud.setPan(it->second.handle, value);
    }

    void SoLoudAPI::SetLooping(UUID sourceID, bool value)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        it->second.state.looping = value;
        if (soloud.isValidVoiceHandle(it->second.handle))
            soloud.setLooping(it->second.handle, value);
    }

    void SoLoudAPI::SetPlaybackSpeed(UUID sourceID, float value)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        it->second.state.playback_speed = value;
        if (soloud.isValidVoiceHandle(it->second.handle))
            soloud.setRelativePlaySpeed(it->second.handle, value);
    }

    void SoLoudAPI::Seek(UUID sourceID, float value)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        if (soloud.isValidVoiceHandle(it->second.handle))
            soloud.seek(it->second.handle, value);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 3D
    // ─────────────────────────────────────────────────────────────────────────

    void SoLoudAPI::SetPosition(UUID sourceID, const glm::vec3& position)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end() || it->second.type != AudioType::Audio3D) return;

        it->second.state.position = position;
        if (soloud.isValidVoiceHandle(it->second.handle))
            soloud.set3dSourcePosition(it->second.handle, position.x, position.y, position.z);
    }

    void SoLoudAPI::SetVelocity(UUID sourceID, const glm::vec3& velocity)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end() || it->second.type != AudioType::Audio3D) return;

        it->second.state.velocity = velocity;
        if (soloud.isValidVoiceHandle(it->second.handle))
            soloud.set3dSourceVelocity(it->second.handle, velocity.x, velocity.y, velocity.z);
    }

    void SoLoudAPI::SetDistance(UUID sourceID, float minDist, float maxDist)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end() || it->second.type != AudioType::Audio3D) return;

        it->second.config.minDistance = minDist;
        it->second.config.maxDistance = maxDist;
        if (soloud.isValidVoiceHandle(it->second.handle))
            soloud.set3dSourceMinMaxDistance(it->second.handle, minDist, maxDist);
    }

    void SoLoudAPI::SetAttenuation(UUID sourceID, AudioAttenuation attenuation)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end() || it->second.type != AudioType::Audio3D) return;

        it->second.config.attenuation = attenuation;
        if (soloud.isValidVoiceHandle(it->second.handle))
            soloud.set3dSourceAttenuation(it->second.handle, ToSoLoudAttenuation(attenuation), 1.0f);
    }

    void SoLoudAPI::UpdateListener(const AudioListener& listener)
    {
        soloud.set3dListenerPosition(listener.position.x, listener.position.y, listener.position.z);
        soloud.set3dListenerVelocity(listener.velocity.x, listener.velocity.y, listener.velocity.z);
        soloud.set3dListenerAt(listener.forward.x, listener.forward.y, listener.forward.z);
        soloud.set3dListenerUp(listener.up.x, listener.up.y, listener.up.z);
        soloud.update3dAudio();
    }
}