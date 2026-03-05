#pragma once
#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include <glm/glm.hpp>
#include <string>

namespace Aether {
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
        float volume = 1.0f, pan = 0.0f, playback_speed = 1.0f;
        glm::vec3 position = glm::vec3(0.0f);
        glm::vec3 velocity = glm::vec3(0.0f);
    };

    class AudioAPI
    {
    public: public:
        enum class API {
            None = 0, SoLoud = 1
        };
    public:
        virtual ~AudioAPI() = default;
        virtual void Init() = 0;
        virtual void Shutdown() = 0;

        virtual void AddSound(UUID soundID, const std::string& path) = 0;
        virtual void RemoveSound(UUID soundID) = 0;

        virtual void CreateSource(UUID sourceID, UUID soundID, AudioType type) = 0;
        virtual void DestroySource(UUID sourceID) = 0;
        virtual bool IsActive(UUID sourceID) = 0;
        
        virtual void Play(UUID sourceID) = 0;
        virtual void Pause(UUID sourceID) = 0;
        virtual void Stop(UUID sourceID) = 0;

        virtual void SetGlobalVolume(float value) = 0;
        virtual void SetMaxActiveSource(uint32_t value) = 0;

        virtual void SetVolume(UUID sourceID, float value) = 0;
        virtual void SetPan(UUID sourceID, float value) = 0;
        virtual void SetLooping(UUID sourceID, bool value) = 0;
        virtual void SetPlaybackSpeed(UUID sourceID, float value) = 0;
        virtual void Seek(UUID sourceID, float value) = 0;

        //3d only
        virtual void SetSpeedSound(float value) = 0;
        virtual void SetPosition(UUID sourceID, const glm::vec3& position) = 0;
        virtual void SetVelocity(UUID sourceID, const glm::vec3& velocity) = 0;
        virtual void SetDistance(UUID sourceID, float minDist, float maxDist) = 0;

        virtual void UpdateListener(const AudioListener& listener) = 0;

        static Scope<AudioAPI> Create();
    private:
        static API s_API;
    };
}