#pragma once

#include "Aether/Audio/AudioAPI.h"

namespace Aether {

    class AETHER_API AudioSystem
    {
    public:
        static void Init() 
        {
            s_AudioAPI->Init();
        }

        static void Shutdown()
        {
            s_AudioAPI->Shutdown();
        }

        static void AddSound(UUID soundID, const std::string& path) 
        {
            s_AudioAPI->AddSound(soundID, path);
        }
        static void RemoveSound(UUID soundID)
        {
            s_AudioAPI->RemoveSound(soundID);
        }

        static void CreateSource(UUID sourceID, UUID soundID, AudioType type)
        {
            s_AudioAPI->CreateSource(sourceID, soundID, type);
        }

        static void DestroySource(UUID sourceID)
        {
            s_AudioAPI->DestroySource(sourceID);
        }

        static const AudioState* GetState(UUID sourceID) 
        {
            return s_AudioAPI->GetState(sourceID);
        }

        static bool IsActive(UUID sourceID)
        {
            return s_AudioAPI->GetState(sourceID);
        }

        static void Play(UUID sourceID) 
        {
            s_AudioAPI->Play(sourceID);
        }

        static void Pause(UUID sourceID)
        {
            s_AudioAPI->Pause(sourceID);
        }

        static void Stop(UUID sourceID)
        {
            s_AudioAPI->Stop(sourceID);
        }

        static void SetGlobalVolume(float value)
        {
            s_AudioAPI->SetGlobalVolume(value);
        }

        static void SetMaxActiveSource(uint32_t value)
        {
            s_AudioAPI->SetMaxActiveSource(value);
        }

        static void SetVolume(UUID sourceID, float value)
        {
            s_AudioAPI->SetVolume(sourceID, value);
        }

        static void SetPan(UUID sourceID, float value)
        {
            s_AudioAPI->SetPan(sourceID, value);
        }

        static void SetLooping(UUID sourceID, bool value)
        {
            s_AudioAPI->SetLooping(sourceID, value);
        }

        static void SetPlaybackSpeed(UUID sourceID, float value)
        {
            s_AudioAPI->SetPlaybackSpeed(sourceID, value);
        }

        static void Seek(UUID sourceID, float value)
        {
            s_AudioAPI->Seek(sourceID, value);
        }

        //3d only
        static void SetSpeedSound(float value)
        {
            s_AudioAPI->SetSpeedSound(value);
        }

        static void SetPosition(UUID sourceID, const glm::vec3& position)
        {
            s_AudioAPI->SetPosition(sourceID, position);
        }

        static void SetVelocity(UUID sourceID, const glm::vec3& velocity) 
        {
            s_AudioAPI->SetVelocity(sourceID, velocity);
        }

        static void SetDistance(UUID sourceID, float minDist, float maxDist)
        {
            s_AudioAPI->SetDistance(sourceID, minDist, maxDist);
        }

        static void UpdateListener(const AudioListener& listener)
        {
            s_AudioAPI->UpdateListener(listener);
        }
    
    private:
        static Scope<AudioAPI> s_AudioAPI;
    };
}