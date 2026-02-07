#include "Platform/Cgltf/GLTF_AnimationParser.h"
#include <cgltf.h>
#include <glm/gtc/type_ptr.hpp>

namespace Aether {

    Ref<SkelAnimInfo> GLTF_AnimationParser::Parsing(void* data)
    {
        return ParseSkelAnim(data);
    }

    Ref<SkelAnimInfo> GLTF_AnimationParser::ParseSkelAnim(void* data)
    {
        cgltf_data* gltf = static_cast<cgltf_data*>(data);
        auto result = CreateRef<SkelAnimInfo>();

        // ===== Parse Skeletons (from skins) =====
        result->skeletons.reserve(gltf->skins_count);
        
        for (size_t skinIdx = 0; skinIdx < gltf->skins_count; skinIdx++)
        {
            const cgltf_skin* skin = &gltf->skins[skinIdx];
            
            SkeletonCreateInfo skelInfo;
            skelInfo.DebugName = skin->name ? skin->name : ("Skeleton_" + std::to_string(skinIdx));
            skelInfo.Joints.reserve(skin->joints_count);
            
            // Read inverse bind matrices
            std::vector<glm::mat4>& inverseBindMatrices = skelInfo.IBM;
            if (skin->inverse_bind_matrices)
            {
                cgltf_accessor* accessor = skin->inverse_bind_matrices;
                size_t matCount = accessor->count;
                inverseBindMatrices.resize(matCount);
                
                for (size_t i = 0; i < matCount; i++)
                {
                    float mat[16];
                    cgltf_accessor_read_float(accessor, i, mat, 16);
                    inverseBindMatrices[i] = glm::make_mat4(mat);
                }
            }
            
            // Build joint hierarchy
            for (size_t jointIdx = 0; jointIdx < skin->joints_count; jointIdx++)
            {
                cgltf_node* jointNode = skin->joints[jointIdx];
                
                SkeletonCreateInfo::Joint joint;
                joint.Name = jointNode->name ? jointNode->name : ("Joint_" + std::to_string(jointIdx));
                
                // Find parent index
                joint.ParentIndex = -1;
                if (jointNode->parent)
                {
                    for (size_t i = 0; i < skin->joints_count; i++)
                    {
                        if (skin->joints[i] == jointNode->parent)
                        {
                            joint.ParentIndex = (int16_t)i;
                            break;
                        }
                    }
                }
                
                // Extract local transform
                if (jointNode->has_translation)
                {
                    joint.Translation = glm::vec3(
                        jointNode->translation[0],
                        jointNode->translation[1],
                        jointNode->translation[2]
                    );
                }
                else
                {
                    joint.Translation = glm::vec3(0.0f);
                }
                
                if (jointNode->has_rotation)
                {
                    joint.Rotation = glm::quat(
                        jointNode->rotation[3],  // w
                        jointNode->rotation[0],  // x
                        jointNode->rotation[1],  // y
                        jointNode->rotation[2]   // z
                    );
                }
                else
                {
                    joint.Rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                }
                
                if (jointNode->has_scale)
                {
                    joint.Scale = glm::vec3(
                        jointNode->scale[0],
                        jointNode->scale[1],
                        jointNode->scale[2]
                    );
                }
                else
                {
                    joint.Scale = glm::vec3(1.0f);
                }
                
                skelInfo.Joints.push_back(joint);
            }
            
            result->skeletons.push_back(skelInfo);
            AE_CORE_INFO("Parsed skeleton: {0} with {1} joints", skelInfo.DebugName, skelInfo.Joints.size());
        }

        // ===== Parse Animation Clips =====
        result->clips.reserve(gltf->animations_count);
        
        for (size_t animIdx = 0; animIdx < gltf->animations_count; animIdx++)
        {
            const cgltf_animation* anim = &gltf->animations[animIdx];
            
            AnimationClipCreateInfo clipInfo;
            clipInfo.DebugName = anim->name ? anim->name : ("Animation_" + std::to_string(animIdx));
            
            // Calculate duration from all samplers
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
            clipInfo.Duration = maxTime;
            clipInfo.SampleRate = 30.0f;  // Default, could be calculated from keyframe density
            
            // Parse channels
            for (size_t chanIdx = 0; chanIdx < anim->channels_count; chanIdx++)
            {
                const cgltf_animation_channel* channel = &anim->channels[chanIdx];
                const cgltf_animation_sampler* sampler = channel->sampler;
                
                if (!channel->target_node || !sampler->input || !sampler->output)
                    continue;
                
                // Find which joint this channel targets (if it's part of a skeleton)
                int jointIndex = -1;
                for (size_t skinIdx = 0; skinIdx < gltf->skins_count; skinIdx++)
                {
                    const cgltf_skin* skin = &gltf->skins[skinIdx];
                    for (size_t jointIdx = 0; jointIdx < skin->joints_count; jointIdx++)
                    {
                        if (skin->joints[jointIdx] == channel->target_node)
                        {
                            jointIndex = (int)jointIdx;
                            break;
                        }
                    }
                    if (jointIndex >= 0) break;
                }
                
                if (jointIndex < 0) continue;  // Not a skeleton joint, skip
                
                // Find or create track for this joint
                AnimationClipCreateInfo::Track* track = nullptr;
                for (auto& t : clipInfo.Tracks)
                {
                    if (t.JointIndex == jointIndex)
                    {
                        track = &t;
                        break;
                    }
                }
                
                if (!track)
                {
                    clipInfo.Tracks.push_back(AnimationClipCreateInfo::Track());
                    track = &clipInfo.Tracks.back();
                    track->JointIndex = jointIndex;
                }
                
                // Read keyframe times
                std::vector<float> times;
                times.resize(sampler->input->count);
                for (size_t i = 0; i < sampler->input->count; i++)
                {
                    cgltf_accessor_read_float(sampler->input, i, &times[i], 1);
                }
                
                // Read keyframe values based on path
                if (channel->target_path == cgltf_animation_path_type_translation)
                {
                    track->TranslationTimes = times;
                    track->TranslationValues.resize(sampler->output->count);
                    
                    for (size_t i = 0; i < sampler->output->count; i++)
                    {
                        float vec[3];
                        cgltf_accessor_read_float(sampler->output, i, vec, 3);
                        track->TranslationValues[i] = glm::vec3(vec[0], vec[1], vec[2]);
                    }
                }
                else if (channel->target_path == cgltf_animation_path_type_rotation)
                {
                    track->RotationTimes = times;
                    track->RotationValues.resize(sampler->output->count);
                    
                    for (size_t i = 0; i < sampler->output->count; i++)
                    {
                        float quat[4];
                        cgltf_accessor_read_float(sampler->output, i, quat, 4);
                        track->RotationValues[i] = glm::quat(quat[3], quat[0], quat[1], quat[2]);  // w,x,y,z
                    }
                }
                else if (channel->target_path == cgltf_animation_path_type_scale)
                {
                    track->ScaleTimes = times;
                    track->ScaleValues.resize(sampler->output->count);
                    
                    for (size_t i = 0; i < sampler->output->count; i++)
                    {
                        float vec[3];
                        cgltf_accessor_read_float(sampler->output, i, vec, 3);
                        track->ScaleValues[i] = glm::vec3(vec[0], vec[1], vec[2]);
                    }
                }
            }
            
            result->clips.push_back(clipInfo);
            AE_CORE_INFO("Parsed animation: {0}, duration: {1}s, {2} tracks", 
                clipInfo.DebugName, clipInfo.Duration, clipInfo.Tracks.size());
        }
        
        return result;
    }

}