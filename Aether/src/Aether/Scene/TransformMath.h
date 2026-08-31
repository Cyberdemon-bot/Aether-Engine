#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Aether {
    namespace Utils
    {
        inline void GetTRS(const glm::mat4& matrix, glm::vec3& outPos, glm::quat& outRot, glm::vec3& outScale)
        {
            outPos = glm::vec3(matrix[3]);
            glm::vec3 col0(matrix[0]);
            glm::vec3 col1(matrix[1]);
            glm::vec3 col2(matrix[2]);
            outScale.x = glm::length(col0);
            outScale.y = glm::length(col1);
            outScale.z = glm::length(col2);
            glm::vec3 x = (outScale.x > 0.00001f) ? (col0 / outScale.x) : glm::vec3(1.0f, 0.0f, 0.0f);
            glm::vec3 y = (outScale.y > 0.00001f) ? (col1 / outScale.y) : glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 z = (outScale.z > 0.00001f) ? (col2 / outScale.z) : glm::vec3(0.0f, 0.0f, 1.0f);
            outRot = glm::quat_cast(glm::mat3(x, y, z));
        }
    }
}