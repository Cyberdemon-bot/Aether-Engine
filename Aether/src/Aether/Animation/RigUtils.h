#pragma once

#include "RigModule.h"

namespace Aether {
    void CalcRestPoseMatrices(const SkeletonCreateInfo& data, glm::mat4* arr, size_t size)
    {
        if (data.Joints.size() > size) return;
        for (size_t i = 0; i < data.Joints.size(); i++)
        {
            const auto& joint = data.Joints[i];
            glm::mat4 T = glm::translate(glm::mat4(1.0f), joint.Translation);
            glm::mat4 R = glm::mat4_cast(joint.Rotation);
            glm::mat4 S = glm::scale(glm::mat4(1.0f), joint.Scale);
            glm::mat4 local = T * R * S;
            int parent = joint.ParentIndex;

            if (parent == -1) arr[i] = local;
            else arr[i] = arr[parent] * local;
        }
    }
}