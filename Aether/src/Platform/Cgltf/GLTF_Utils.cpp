#include "Platform/Cgltf/GLTF_Utils.h"
#include "aepch.h"

namespace Aether {
    void ReadAccessorFloat(const cgltf_accessor* accessor, std::vector<float>& data)
    {
        if (!accessor) return;

        size_t floatCount = accessor->count * cgltf_num_components(accessor->type);
        data.resize(floatCount);
        
        cgltf_accessor_unpack_floats(accessor, data.data(), floatCount);
    }

    void ReadAccessorUint(const cgltf_accessor* accessor, std::vector<uint32_t>& data)
    {
        if (!accessor) return;

        cgltf_size num_components = cgltf_num_components(accessor->type);
        size_t uintCount = accessor->count * num_components;
        data.resize(uintCount);
        
        for (cgltf_size i = 0; i < accessor->count; ++i) 
            cgltf_accessor_read_uint(accessor, i, &data[i * num_components], num_components);
    }
}