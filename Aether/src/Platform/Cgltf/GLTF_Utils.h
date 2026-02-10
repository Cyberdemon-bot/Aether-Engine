#include <vector>
#include <cgltf.h>
#include <glm/glm.hpp>

namespace Aether {
    void ReadAccessorFloat(const cgltf_accessor* accessor, std::vector<float>& data);
    void ReadAccessorUint(const cgltf_accessor* accessor, std::vector<uint32_t>& data);
    void ReadAccessorFloatToMat(const cgltf_accessor* accessor, std::vector<glm::mat4>& data);
}