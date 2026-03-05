#include "aepch.h"
#include "Platform/SoLoud/SoLoudAPI.h"

namespace Aether {
    void SoLoudAPI::Init()
    {
        soloud.init();
        m_Sources.reserve(32);
    }

    void SoLoudAPI::Shutdown()
    {
        soloud.stopAll();
        m_Sounds.clear();
        m_Sources.clear();
        soloud.deinit();
    }

    void SoLoudAPI::AddSound(UUID soundID, const std::string& path) 
    {
        if (m_Sounds.find(soundID) != m_Sounds.end()) return;

        auto& sound = m_Sounds[soundID];
        SoLoud::result res = sound.load(path.c_str());
        if (res != SoLoud::SO_NO_ERROR)
        {
            m_Sounds.erase(soundID);
            return;
        }
    }
    void SoLoudAPI::RemoveSound(UUID soundID) 
    {
        auto it = m_Sounds.find(soundID);
        if (it == m_Sounds.end()) return;

        for (auto& [id, src] : m_Sources)
        {
            if (src.sound == &it->second)
                Stop(id);
        }
        m_Sounds.erase(soundID);
    }

    void SoLoudAPI::CreateSource(UUID sourceID, UUID soundID, AudioType type)
    {
        if (m_Sources.find(sourceID) != m_Sources.end()) return;
        if (m_Sounds.find(soundID) == m_Sounds.end()) return;

        auto& source = m_Sources[sourceID];
        source.sound = &m_Sounds[soundID];
        source.type = type;
    }

    void SoLoudAPI::DestroySource(UUID sourceID)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        if (soloud.isValidVoiceHandle(it->second.handle)) 
            soloud.stop(it->second.handle);
        m_Sources.erase(sourceID);
    }

    const AudioState* SoLoudAPI::GetState(UUID sourceID) const
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return nullptr;

        return &it->second.state;
    }

    bool SoLoudAPI::IsActive(UUID sourceID)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return false;

        return soloud.isValidVoiceHandle(it->second.handle);
    }

    void SoLoudAPI::Play(UUID sourceID)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        if (!soloud.isValidVoiceHandle(it->second.handle)) 
        {
            if (it->second.type == AudioType::Audio2D) it->second.handle = soloud.play(*it->second.sound);
            else it->second.handle = soloud.play3d(*it->second.sound, 0, 0, 0);
        }
        
        it->second.state.pausing = false;
        int handle = it->second.handle;
        soloud.setPause(handle, it->second.state.pausing);
        soloud.setVolume(handle, it->second.state.volume);
        soloud.setPan(handle, it->second.state.pan);
        soloud.setRelativePlaySpeed(handle, it->second.state.playback_speed);
        soloud.setLooping(handle, it->second.state.looping);

        if (it->second.type == AudioType::Audio3D)
        {
            float x = it->second.state.position.x;
            float y = it->second.state.position.y;
            float z = it->second.state.position.z;
            float vx = it->second.state.velocity.x;
            float vy = it->second.state.velocity.y;
            float vz = it->second.state.velocity.z;
            float mindist = it->second.config.minDistance;
            float maxdist = it->second.config.maxDistance;
            SoLoud::AudioSource::ATTENUATION_MODELS atten;
            switch(it->second.config.attenuation)
            {
                case AudioAttenuation::NO_ATTENUATION:
                    atten = SoLoud::AudioSource::NO_ATTENUATION;
                    break;
                case AudioAttenuation::INVERSE_DISTANCE:
                    atten = SoLoud::AudioSource::INVERSE_DISTANCE;
                    break;
                case AudioAttenuation::LINEAR_DISTANCE:
                    atten = SoLoud::AudioSource::LINEAR_DISTANCE;
                    break;
                case AudioAttenuation::EXPONENTIAL_DISTANCE:
                    atten = SoLoud::AudioSource::EXPONENTIAL_DISTANCE;
                    break;
            }

            soloud.set3dSourceAttenuation(handle, atten, 1.0f);
            soloud.set3dSourceMinMaxDistance(handle, mindist, maxdist);
            soloud.set3dSourceVelocity(handle, vx, vy, vz);
            soloud.set3dSourcePosition(handle, x, y, z);
        }
    }

    void SoLoudAPI::Pause(UUID sourceID) 
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        it->second.state.pausing = true;
        int handle = it->second.handle;
        if (soloud.isValidVoiceHandle(handle)) 
            soloud.setPause(handle, true);
    }

    void SoLoudAPI::Stop(UUID sourceID)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        if (soloud.isValidVoiceHandle(it->second.handle)) 
            soloud.stop(it->second.handle);
    }

    void SoLoudAPI::SetVolume(UUID sourceID, float value)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        it->second.state.volume = value;
        int handle = it->second.handle;
        if (soloud.isValidVoiceHandle(handle)) 
            soloud.setVolume(handle, value);
    }

    void SoLoudAPI::SetPan(UUID sourceID, float value) 
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        it->second.state.pan = value;
        int handle = it->second.handle;
        if (soloud.isValidVoiceHandle(handle)) 
            soloud.setPan(handle, value);
    }

    void SoLoudAPI::SetLooping(UUID sourceID, bool value)
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        it->second.state.looping = value;
        int handle = it->second.handle;
        if (soloud.isValidVoiceHandle(handle)) 
            soloud.setLooping(handle, value);
    }

    void SoLoudAPI::SetPlaybackSpeed(UUID sourceID, float value) 
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        it->second.state.playback_speed = value;
        int handle = it->second.handle;
        if (soloud.isValidVoiceHandle(handle)) 
            soloud.setRelativePlaySpeed(handle, value);
    }
    void SoLoudAPI::Seek(UUID sourceID, float value) 
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;

        int handle = it->second.handle;
        if (soloud.isValidVoiceHandle(handle)) 
            soloud.seek(handle, value);
    }

    void SoLoudAPI::SetPosition(UUID sourceID, const glm::vec3& position) 
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;
        if (it->second.type != AudioType::Audio3D) return;

        it->second.state.position = position;
        int handle = it->second.handle;
        if (soloud.isValidVoiceHandle(handle)) 
            soloud.set3dSourcePosition(handle, position.x, position.y, position.z);
    }

    void SoLoudAPI::SetVelocity(UUID sourceID, const glm::vec3& velocity) 
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;
        if (it->second.type != AudioType::Audio3D) return;

        it->second.state.velocity = velocity;
        int handle = it->second.handle;
        if (soloud.isValidVoiceHandle(handle)) 
            soloud.set3dSourceVelocity(handle, velocity.x, velocity.y, velocity.z);
    }

    void SoLoudAPI::SetDistance(UUID sourceID, float minDist, float maxDist) 
    {
        auto it = m_Sources.find(sourceID);
        if (it == m_Sources.end()) return;
        if (it->second.type != AudioType::Audio3D) return;

        it->second.config.minDistance = minDist;
        it->second.config.maxDistance = maxDist;
        int handle = it->second.handle;
        if (soloud.isValidVoiceHandle(handle)) 
            soloud.set3dSourceMinMaxDistance(handle, minDist, maxDist);
    }

    void SoLoudAPI::UpdateListener(const AudioListener& listener)
    {
        soloud.set3dListenerPosition(
            listener.position.x,
            listener.position.y,
            listener.position.z
        );

        soloud.set3dListenerVelocity(
            listener.velocity.x,
            listener.velocity.y,
            listener.velocity.z
        );

        soloud.set3dListenerAt(
            listener.forward.x,
            listener.forward.y,
            listener.forward.z
        );

        soloud.set3dListenerUp(
            listener.up.x,
            listener.up.y,
            listener.up.z
        );

        soloud.update3dAudio();
    }

}