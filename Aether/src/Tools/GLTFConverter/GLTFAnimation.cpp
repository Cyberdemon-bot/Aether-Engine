#include "aepch.h"
#include "GLTFConverter.h"
#include "GLTFUtils.h"
#include "Aether/Core/Assert.h"

#include <cgltf.h>
#include <unordered_map>
#include <map>

namespace Aether {

    void GLTFConverter::ParseRigs(cgltf_data* gltf, ParsedScene& scene)
    {
        scene.Skeletons.reserve(gltf->skins_count);

        for (size_t skinIdx = 0; skinIdx < gltf->skins_count; skinIdx++)
        {
            const cgltf_skin* skin = &gltf->skins[skinIdx];

            ASkeletonCreateInfo skelInfo;
            skelInfo.id = UUID();
            skelInfo.debugName = skin->name ? skin->name : ("Skeleton_" + std::to_string(skinIdx));
            skelInfo.data.Joints.resize(skin->joints_count);

            std::unordered_map<cgltf_node*, int16_t> nodeMap;
            for (size_t i = 0; i < skin->joints_count; i++)
                nodeMap[skin->joints[i]] = (int16_t)i;

            if (skin->inverse_bind_matrices)
                GLTFUtils::ReadAccessorFloatToMat4(skin->inverse_bind_matrices, skelInfo.data.IBM);
            else
                skelInfo.data.IBM.resize(skin->joints_count, glm::mat4(1.0f));

            for (size_t jointIdx = 0; jointIdx < skin->joints_count; jointIdx++)
            {
                cgltf_node* jointNode = skin->joints[jointIdx];

                SkeletonCreateInfo::Joint joint;
                joint.Name = jointNode->name ? jointNode->name : ("Joint_" + std::to_string(jointIdx));

                joint.ParentIndex = -1;
                if (jointNode->parent)
                {
                    auto it = nodeMap.find(jointNode->parent);
                    if (it != nodeMap.end())
                        joint.ParentIndex = it->second;
                }

                joint.Translation = jointNode->has_translation
                    ? glm::vec3(jointNode->translation[0], jointNode->translation[1], jointNode->translation[2])
                    : glm::vec3(0.0f);

                joint.Rotation = jointNode->has_rotation
                    ? glm::quat(jointNode->rotation[3], jointNode->rotation[0], jointNode->rotation[1], jointNode->rotation[2])
                    : glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

                joint.Scale = jointNode->has_scale
                    ? glm::vec3(jointNode->scale[0], jointNode->scale[1], jointNode->scale[2])
                    : glm::vec3(1.0f);

                skelInfo.data.Joints[jointIdx] = joint;
            }

            AE_CORE_INFO("GLTFConverter: parsed skeleton '{0}' with {1} joints", skelInfo.debugName, skelInfo.data.Joints.size());
            scene.Skeletons.push_back(std::move(skelInfo));
        }
    }

    void GLTFConverter::ParseClips(cgltf_data* gltf, ParsedScene& scene)
    {
        // Map each joint node to (skeleton index, joint index) so animation
        // channels can be grouped by the skeleton they drive. Relies on
        // scene.Skeletons already being filled in skin order (ParseRigs
        // must run first - enforced by ParseAnimations calling them in
        // that order).
        struct NodeInfo { int skeletonIdx; int jointIdx; };
        std::unordered_map<cgltf_node*, NodeInfo> nodeMap;

        for (size_t i = 0; i < gltf->skins_count; i++)
            for (size_t j = 0; j < gltf->skins[i].joints_count; j++)
                nodeMap[gltf->skins[i].joints[j]] = { (int)i, (int)j };

        for (size_t animIdx = 0; animIdx < gltf->animations_count; animIdx++)
        {
            const cgltf_animation* anim = &gltf->animations[animIdx];

            std::map<int, std::map<cgltf_node*, ClipCreateInfo::Track>> skeletonTracks;
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

                auto it = nodeMap.find(channel->target_node);
                if (it == nodeMap.end()) continue;

                int skeletonIdx = it->second.skeletonIdx;
                int jointIndex = it->second.jointIdx;

                ClipCreateInfo::Track& track = skeletonTracks[skeletonIdx][channel->target_node];
                track.JointIndex = jointIndex;

                if (channel->target_path == cgltf_animation_path_type_translation)
                {
                    GLTFUtils::ReadAccessorFloat(sampler->input, track.TranslationTimes);
                    GLTFUtils::ReadAccessorFloatToVec3(sampler->output, track.TranslationValues);
                }
                else if (channel->target_path == cgltf_animation_path_type_rotation)
                {
                    GLTFUtils::ReadAccessorFloat(sampler->input, track.RotationTimes);
                    GLTFUtils::ReadAccessorFloatToQuat(sampler->output, track.RotationValues);
                }
                else if (channel->target_path == cgltf_animation_path_type_scale)
                {
                    GLTFUtils::ReadAccessorFloat(sampler->input, track.ScaleTimes);
                    GLTFUtils::ReadAccessorFloatToVec3(sampler->output, track.ScaleValues);
                }
            }

            for (auto& [skeletonIdx, trackMap] : skeletonTracks)
            {
                AClipCreateInfo clipInfo;
                clipInfo.id = UUID();

                clipInfo.debugName = (skeletonTracks.size() > 1)
                    ? (anim->name ? std::string(anim->name) + "_Rig" + std::to_string(skeletonIdx)
                                  : "Animation_" + std::to_string(animIdx) + "_Rig" + std::to_string(skeletonIdx))
                    : (anim->name ? anim->name : ("Animation_" + std::to_string(animIdx)));

                clipInfo.data.Duration = maxTime;
                clipInfo.data.SampleRate = 30.0f;

                // Resolved directly to the skeleton's real UUID - no local
                // rig index carried forward for Upload() to resolve later.
                clipInfo.skeleton = scene.Skeletons[skeletonIdx].id;

                for (auto& [node, trackData] : trackMap)
                {
                    if (trackData.TranslationTimes.empty() &&
                        trackData.RotationTimes.empty() &&
                        trackData.ScaleTimes.empty())
                        continue;

                    clipInfo.data.Tracks.push_back(trackData);
                }

                AE_CORE_INFO("GLTFConverter: parsed animation '{0}', duration {1}s, {2} tracks (skeleton {3})",
                    clipInfo.debugName, clipInfo.data.Duration, clipInfo.data.Tracks.size(), skeletonIdx);

                scene.Clips.push_back(std::move(clipInfo));
            }
        }
    }
}