#include "Platform/Cgltf/GLTF_Utils.h"
#include "aepch.h"
#include <glm/gtc/type_ptr.hpp>

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

    void ReadAccessorFloatToMat(const cgltf_accessor* accessor, std::vector<glm::mat4>& data)
    {
        size_t matCount = accessor->count;
        data.resize(matCount);
        
        for (size_t i = 0; i < matCount; i++)
        {
            float mat[16];
            cgltf_accessor_read_float(accessor, i, mat, 16);
            data[i] = glm::make_mat4(mat);
        }
    }
}