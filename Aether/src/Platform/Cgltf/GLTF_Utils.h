#include <vector>
#include <cgltf.h>
#include <glm/glm.hpp>

namespace Aether {
    void ReadAccessorFloat(const cgltf_accessor* accessor, std::vector<float>& data);
    void ReadAccessorUint(const cgltf_accessor* accessor, std::vector<uint32_t>& data);
    void ReadAccessorFloatToMat4(const cgltf_accessor* accessor, std::vector<glm::mat4>& data);
    void ReadAccessorFloatToVec3(const cgltf_accessor* accessor, std::vector<glm::vec3>& data);
    void ReadAccessorFloatToQuat(const cgltf_accessor* accessor, std::vector<glm::quat>& data);
}