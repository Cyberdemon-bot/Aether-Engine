#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/Delegate.h"
#include "Aether/Container/ResourcePool.h"
#include <glm/glm.hpp>
#include <string>

namespace Aether {

    struct AudioSource; 
    struct AudioPlayer; 

    enum class AudioType
    {
        Audio2D, Audio3D
    };

    enum class AudioAttenuation
    {
        NO_ATTENUATION,
        LINEAR_DISTANCE,
        INVERSE_DISTANCE,
        EXPONENTIAL_DISTANCE
    };

    enum AudioDirtyFlags : uint16_t 
    {
        DIRTY_NONE = 0,
        DIRTY_VOLUME = 1 << 0,
        DIRTY_PAN = 1 << 1,
        DIRTY_LOOP = 1 << 2,
        DIRTY_PAUSE = 1 << 3,
        DIRTY_SPEED = 1 << 4,
        DIRTY_3D_POS = 1 << 5,
        DIRTY_3D_VEL = 1 << 6,
        DIRTY_3D_DIST = 1 << 7,
        DIRTY_3D_ATT = 1 << 8,
        DIRTY_ALL = 0xFFFF
    };

    struct AudioListener
    {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec3 forward;
        glm::vec3 up;
    };

    struct Audio3DInfo
    {
        float minDistance = 1.0f;
        float maxDistance = 50.0f;
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 velocity = glm::vec3(0.0f);
        AudioAttenuation attenuation = AudioAttenuation::INVERSE_DISTANCE;
    };

    struct AudioState
    {
        bool looping = false;
        bool pausing = false;
        float volume = 1.0f;
        float pan = 0.0f;
        float playback_speed = 1.0f;
        Audio3DInfo _3dinfo;
    };

    struct PlayerEditProxy
    {
        AudioState temp;
        uint16_t dirty_flags = AudioDirtyFlags::DIRTY_NONE;

        PlayerEditProxy(const AudioState& currentState) 
            : temp(currentState) {}

        PlayerEditProxy& SetVolume(float v) 
        {
            if (temp.volume == v) return *this;

            temp.volume = v;
            dirty_flags |= AudioDirtyFlags::DIRTY_VOLUME;
            return *this;
        }
        PlayerEditProxy& SetPan(float v) 
        {
            if (temp.pan == v) return *this;

            temp.pan = v;
            dirty_flags |= AudioDirtyFlags::DIRTY_PAN;
            return *this;
        }
        PlayerEditProxy& SetSpeed(float v) 
        {
            if (temp.playback_speed == v) return *this;

            temp.playback_speed = v;
            dirty_flags |= AudioDirtyFlags::DIRTY_SPEED;
            return *this;
        }
        PlayerEditProxy& SetLoop(bool v) 
        {
            if (temp.looping == v) return *this;

            temp.looping = v;
            dirty_flags |= AudioDirtyFlags::DIRTY_LOOP;
            return *this;
        }
        PlayerEditProxy& SetPause(bool v) 
        {
            if (temp.pausing == v) return *this;

            temp.pausing = v;
            dirty_flags |= AudioDirtyFlags::DIRTY_PAUSE;
            return *this;
        }
        PlayerEditProxy& SetPos(const glm::vec3& v) 
        {
            if (temp._3dinfo.position == v) return *this;

            temp._3dinfo.position = v;
            dirty_flags |= AudioDirtyFlags::DIRTY_3D_POS;
            return *this;
        }
        PlayerEditProxy& SetVel(const glm::vec3& v) 
        {
            if (temp._3dinfo.velocity == v) return *this;

            temp._3dinfo.velocity = v;
            dirty_flags |= AudioDirtyFlags::DIRTY_3D_VEL;
            return *this;
        }
        PlayerEditProxy& SetDist(float min, float max) 
        {
            if (temp._3dinfo.minDistance == min && temp._3dinfo.maxDistance == max) return *this;

            temp._3dinfo.minDistance = min;
            temp._3dinfo.maxDistance = max;
            dirty_flags |= AudioDirtyFlags::DIRTY_3D_DIST;
            return *this;
        }
        PlayerEditProxy& SetAttenuation(AudioAttenuation v) 
        {
            if (temp._3dinfo.attenuation == v) return *this;

            temp._3dinfo.attenuation = v;
            dirty_flags |= AudioDirtyFlags::DIRTY_3D_ATT;
            return *this;
        }
    };

    class AudioAPI
    {
    public:
        virtual ~AudioAPI() = default;

        virtual void Init() = 0;
        virtual void Shutdown() = 0;
        virtual void Update() = 0;
        
        virtual Handle<AudioSource> CreateSource(const std::string& path) = 0;
        virtual void DestroySource(Handle<AudioSource> handle) = 0;
       
        virtual Handle<AudioPlayer> CreatePlayer(Handle<AudioSource> source, AudioType type) = 0;
        virtual void DestroyPlayer(Handle<AudioPlayer> handle) = 0;

        virtual void Play(Handle<AudioPlayer> handle) = 0;
        virtual void Pause(Handle<AudioPlayer> handle) = 0;
        virtual void Stop(Handle<AudioPlayer> handle) = 0;
        virtual void Seek(Handle<AudioPlayer> handle, float value) = 0;

        virtual bool Modify(Handle<AudioPlayer> handle, Delegate<void(PlayerEditProxy&)> modifier) = 0;
        virtual void UpdateListener(const AudioListener& listener) = 0;

        static Scope<AudioAPI> Create();
    private:
        enum class API 
        {
            None = 0, SoLoud = 1
        };
        static API s_API;

        static API GetAPI() 
        { 
            return s_API; 
        }
    };
}