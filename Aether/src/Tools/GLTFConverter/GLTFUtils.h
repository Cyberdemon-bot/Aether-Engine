#pragma once

#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct cgltf_accessor;

namespace Aether::GLTFUtils {

    void ReadAccessorFloat(const cgltf_accessor* accessor, std::vector<float>& data);
    void ReadAccessorUint(const cgltf_accessor* accessor, std::vector<uint32_t>& data);
    void ReadAccessorFloatToMat4(const cgltf_accessor* accessor, std::vector<glm::mat4>& data);
    void ReadAccessorFloatToVec3(const cgltf_accessor* accessor, std::vector<glm::vec3>& data);
    void ReadAccessorFloatToQuat(const cgltf_accessor* accessor, std::vector<glm::quat>& data);
}