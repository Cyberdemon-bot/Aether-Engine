#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include <string>

namespace Aether {
    struct KeyFrame
    {
        float time;
        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;

        KeyFrame() 
            : time(0.0f)
            , translation(0.0f)
            , rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f))  
            , scale(1.0f) 
        {}

        glm::mat4 CalcMat() const //keyframe to mat4
        {
            glm::mat4 TMat = glm::translate(glm::mat4(1.0f), translation);
            glm::mat4 RMat = glm::toMat4(rotation);
            glm::mat4 SMat = glm::scale(glm::mat4(1.0f), scale);

            return TMat * RMat * SMat;
        }
    };

    struct Channel
    {
        int32_t boneIdx;
        std::vector<KeyFrame> keyframes;

        glm::mat4 Sample(float time) const
        {
            if (keyframes.empty()) return glm::mat4(1.0f);
            if (keyframes.size() == 1 || time <= keyframes[0].time) return keyframes[0].CalcMat();
            if (time >= keyframes.back().time) return keyframes.back().CalcMat();

            int nextIdx = 0, prevIdx = 0;
            for (int i = 0; i < keyframes.size(); i++)
                if (keyframes[i].time > time) 
                {
                    nextIdx = i;
                    prevIdx = i - 1;
                    break;
                }
            
            const KeyFrame& next = keyframes[nextIdx];
            const KeyFrame& prev = keyframes[prevIdx];

            float duration = next.time - prev.time;
            float t = (time - prev.time) / duration;

            glm::vec3 translation = glm::mix(prev.translation, next.translation, t);
            glm::quat rotation = glm::slerp(prev.rotation, next.rotation, t);
            glm::vec3 scale = glm::mix(prev.scale, next.scale, t);

            glm::mat4 TMat = glm::translate(glm::mat4(1.0f), translation);
            glm::mat4 RMat = glm::toMat4(rotation);
            glm::mat4 SMat = glm::scale(glm::mat4(1.0f), scale);

            return TMat * RMat * SMat;
        }   
    };

    struct Clip
    {
        float Durations;
        std::vector<Channel> Channels;
        
        Clip() = default;
        Clip(const std::vector<Channel>& channels, float duration = 0.0f) : Durations(duration), Channels(channels) {}

        const Channel* FindChannel(int32_t boneIdx) const
        {
            for (const auto& channel : Channels)
            {
                if (channel.boneIdx == boneIdx)
                    return &channel;
            }
            return nullptr;
        }
    };

    class AETHER_API ClipLibrary
    {
    public:
        void Init();
        void Shutdown();

        static void Add(Ref<Clip> obj, UUID id);
        static Ref<Clip> Get(UUID id);
        
        static bool Exists(UUID id);
    private:
        static std::unordered_map<UUID, Ref<Clip>>& GetClips();
    };

}