#pragma once
#include "Aether/Core/Base.h"
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

        virtual AudioState* GetState(Handle<AudioPlayer> player) = 0;
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