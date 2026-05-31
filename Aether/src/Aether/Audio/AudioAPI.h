#pragma once
#include "Aether/Core/Base.h"
#include "Aether/Container/ResourcePool.h"
#include <glm/glm.hpp>
#include <string>

namespace Aether {

    struct AudioSource; 

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

    struct AudioListener
    {
        glm::vec3 position;
        glm::vec3 velocity;
        glm::vec3 forward;
        glm::vec3 up;
    };

    struct Audio3DConfig
    {
        float minDistance = 1.0f;
        float maxDistance = 50.0f;
        AudioAttenuation attenuation = AudioAttenuation::INVERSE_DISTANCE;
    };

    struct AudioState
    {
        bool looping = false;
        bool pausing = false;
        float volume = 1.0f;
        float pan = 0.0f;
        float playback_speed = 1.0f;
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 velocity = glm::vec3(0.0f);
    };

    class AudioAPI
    {
    public:
        enum class API {
            None = 0, SoLoud = 1
        };

        virtual ~AudioAPI() = default;
        virtual void Init() = 0;
        virtual void Shutdown() = 0;

        virtual Handle<AudioSource> CreateSource(const std::string& path, AudioType type) = 0;
        virtual void DestroySource(Handle<AudioSource> handle) = 0;
        virtual bool IsActive(Handle<AudioSource> handle) = 0;

        virtual void Play(Handle<AudioSource> handle) = 0;
        virtual void Pause(Handle<AudioSource> handle) = 0;
        virtual void Stop(Handle<AudioSource> handle) = 0;

        virtual void SetGlobalVolume(float value) = 0;
        virtual void SetMaxActiveSource(uint32_t value) = 0;

        virtual void SetVolume(Handle<AudioSource> handle, float value) = 0;
        virtual void SetPan(Handle<AudioSource> handle, float value) = 0;
        virtual void SetLooping(Handle<AudioSource> handle, bool value) = 0;
        virtual void SetPlaybackSpeed(Handle<AudioSource> handle, float value) = 0;
        virtual void Seek(Handle<AudioSource> handle, float value) = 0;

        // 3D only
        virtual void SetSpeedSound(float value) = 0;
        virtual void SetPosition(Handle<AudioSource> handle, const glm::vec3& position) = 0;
        virtual void SetVelocity(Handle<AudioSource> handle, const glm::vec3& velocity) = 0;
        virtual void SetDistance(Handle<AudioSource> handle, float minDist, float maxDist) = 0;
        virtual void SetAttenuation(Handle<AudioSource> handle, AudioAttenuation attenuation) = 0;

        virtual void UpdateListener(const AudioListener& listener) = 0;

        static API GetAPI() { return s_API; }
        static Scope<AudioAPI> Create();

    private:
        static API s_API;
    };
}