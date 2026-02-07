#include <vector>
#include <cgltf.h>

namespace Aether {
    void ReadAccessorFloat(const cgltf_accessor* accessor, std::vector<float>& data);
    void ReadAccessorUint(const cgltf_accessor* accessor, std::vector<uint32_t>& data);
}