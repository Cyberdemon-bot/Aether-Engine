#include "Platform/Cgltf/GLTF_AnimationParser.h"
#include "Platform/Cgltf/GLTF_Utils.h"
#include <cgltf.h>
#include <glm/gtc/type_ptr.hpp>
#include <aepch.h>

namespace Aether {

    Ref<RigAnimsCreateInfo> GLTF_AnimationParser::ParseRigAnim(void* data)
    {
        auto result = CreateRef<RigAnimsCreateInfo>();
        ParseRigs(data, result);
        ParseClips(data, result);
        return result;
    }

    void GLTF_AnimationParser::ParseRigs(void* data, Ref<RigAnimsCreateInfo> result)
    {
        cgltf_data* gltf = static_cast<cgltf_data*>(data);
        result->rigs.reserve(gltf->skins_count);

        for (size_t skinIdx = 0; skinIdx < gltf->skins_count; skinIdx++)
        {
            const cgltf_skin* skin = &gltf->skins[skinIdx];
            
            LSkeletonCreateInfo rigInfo;
            rigInfo.AssetID = UUID();
            rigInfo.DebugName = skin->name ? skin->name : ("Skeleton_" + std::to_string(skinIdx));
            rigInfo.spec.Joints.resize(skin->joints_count);

            std::unordered_map<cgltf_node*, int16_t> node_map;
            for (size_t i = 0; i < skin->joints_count; i++) node_map[skin->joints[i]] = (int16_t)i;
            
            if (skin->inverse_bind_matrices)
            {
                cgltf_accessor* accessor = skin->inverse_bind_matrices;
                ReadAccessorFloatToMat4(accessor, rigInfo.spec.IBM);
            }
            else
            {
                rigInfo.spec.IBM.resize(skin->joints_count);
                for(auto& mat : rigInfo.spec.IBM) mat = glm::mat4(1.0f);
            }
        
            for (size_t jointIdx = 0; jointIdx < skin->joints_count; jointIdx++)
            {
                cgltf_node* jointNode = skin->joints[jointIdx];
                
                SkeletonCreateInfo::Joint joint;
                joint.Name = jointNode->name ? jointNode->name : ("Joint_" + std::to_string(jointIdx));
                
                joint.ParentIndex = -1;
                if (jointNode->parent) 
                {
                    auto it = node_map.find(jointNode->parent);
                    if (it != node_map.end())
                        joint.ParentIndex = it->second;
                }
                
                if (jointNode->has_translation)
                {
                    joint.Translation = glm::vec3(
                        jointNode->translation[0],
                        jointNode->translation[1],
                        jointNode->translation[2]
                    );
                }
                else joint.Translation = glm::vec3(0.0f);
                
                if (jointNode->has_rotation)
                {
                    joint.Rotation = glm::quat(
                        jointNode->rotation[3],  // w
                        jointNode->rotation[0],  // x
                        jointNode->rotation[1],  // y
                        jointNode->rotation[2]   // z
                    );
                }
                else joint.Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                
                if (jointNode->has_scale)
                {
                    joint.Scale = glm::vec3(
                        jointNode->scale[0],
                        jointNode->scale[1],
                        jointNode->scale[2]
                    );
                }
                else joint.Scale = glm::vec3(1.0f);
                
                rigInfo.spec.Joints[jointIdx] = joint;
            }
            
            result->rigs.push_back(rigInfo);
            AE_CORE_INFO("Parsed skeleton: {0} with {1} joints", rigInfo.DebugName, rigInfo.spec.Joints.size());
        }
    }

    void GLTF_AnimationParser::ParseClips(void* data, Ref<RigAnimsCreateInfo> result)
    {
        cgltf_data* gltf = static_cast<cgltf_data*>(data);

        struct NodeInfo { int rigIdx, jointIdx; };
        std::unordered_map<cgltf_node*, NodeInfo> node_map;

        for (size_t i = 0; i < gltf->skins_count; i++)
            for (size_t j = 0; j < gltf->skins[i].joints_count; j++)
                node_map[gltf->skins[i].joints[j]] = { (int)i, (int)j };

        for (size_t animIdx = 0; animIdx < gltf->animations_count; animIdx++)
        {
            const cgltf_animation* anim = &gltf->animations[animIdx];
            
            std::map<int, std::map<cgltf_node*, ClipCreateInfo::Track>> rigTracks;
            float maxTime = 0.0f;
            
            for (size_t sampIdx = 0; sampIdx < anim->samplers_count; sampIdx++)
            {
                const cgltf_animation_sampler* sampler = &anim->samplers[sampIdx];
                if (sampler->input && sampler->input->count > 0)
                {
                    float lastTime = 0.0f;
                    cgltf_accessor_read_float(sampler->input, sampler->input->count - 1, &lastTime, 1);
                    maxTime = glm::max(maxTime, lastTime);
                }
            }
            
            for (size_t chanIdx = 0; chanIdx < anim->channels_count; chanIdx++)
            {
                const cgltf_animation_channel* channel = &anim->channels[chanIdx];
                const cgltf_animation_sampler* sampler = channel->sampler;
                
                if (!channel->target_node || !sampler->input || !sampler->output)
                    continue;
                
                auto it = node_map.find(channel->target_node);
                if (it == node_map.end()) continue;

                int rigIdx = it->second.rigIdx;
                int jointIndex = it->second.jointIdx;
                
                ClipCreateInfo::Track& track = rigTracks[rigIdx][channel->target_node];
                track.JointIndex = jointIndex;
                
                if (channel->target_path == cgltf_animation_path_type_translation)
                {
                    ReadAccessorFloat(sampler->input, track.TranslationTimes);
                    ReadAccessorFloatToVec3(sampler->output, track.TranslationValues);
                }
                else if (channel->target_path == cgltf_animation_path_type_rotation)
                {
                    ReadAccessorFloat(sampler->input, track.RotationTimes);
                    ReadAccessorFloatToQuat(sampler->output, track.RotationValues);
                }
                else if (channel->target_path == cgltf_animation_path_type_scale)
                {
                    ReadAccessorFloat(sampler->input, track.ScaleTimes);
                    ReadAccessorFloatToVec3(sampler->output, track.ScaleValues);
                }
            }
            
            for (auto& [rigIdx, track_map] : rigTracks)
            {
                LClipCreateInfo clipInfo;
                
                if (rigTracks.size() > 1)
                {
                    clipInfo.DebugName = anim->name ? 
                        std::string(anim->name) + "_Rig" + std::to_string(rigIdx) :
                        "Animation_" + std::to_string(animIdx) + "_Rig" + std::to_string(rigIdx);
                }
                else
                {
                    clipInfo.DebugName = anim->name ? anim->name : ("Animation_" + std::to_string(animIdx));
                }
                
                clipInfo.AssetID = UUID();
                clipInfo.spec.Duration = maxTime;
                clipInfo.spec.SampleRate = 30.0f;
                clipInfo.rigIdx = rigIdx;
                
                for (auto& [node, trackData] : track_map) 
                {
                    if (trackData.TranslationTimes.empty() && 
                        trackData.RotationTimes.empty() && 
                        trackData.ScaleTimes.empty())
                        continue;
                    clipInfo.spec.Tracks.push_back(trackData);
                }
                
                uint32_t clipIdx = (uint32_t)result->clips.size();
                result->clips.push_back(clipInfo);
                
                AE_CORE_INFO("Parsed animation: {0}, duration: {1}s, {2} tracks for rig {3}", 
                    clipInfo.DebugName, clipInfo.spec.Duration, clipInfo.spec.Tracks.size(), rigIdx);
            }
        }
    }
}