#include "aepch.h"
#include "GLTFConverter.h"
#include "Aether/Core/Assert.h"

#include <cgltf.h>

namespace Aether {

    ParsedScene GLTFConverter::Import(const FileData& data)
    {
        ParsedScene scene;

        cgltf_options options = {};
        cgltf_data* gltf = nullptr;
        cgltf_result result = cgltf_parse(&options, data.bytes, data.size, &gltf);

        if (result != cgltf_result_success)
        {
            AE_CORE_ERROR("GLTFConverter: failed to parse glTF/GLB data");
            return scene;
        }

        result = cgltf_load_buffers(&options, gltf, nullptr);
        if (result != cgltf_result_success)
        {
            AE_CORE_ERROR("GLTFConverter: failed to load glTF/GLB buffers");
            cgltf_free(gltf);
            return scene;
        }

        ParseMaterials(gltf, scene);   
        ParseAnimations(gltf, scene);   
        ParseMeshes(gltf, scene);      
        scene.Hierarchy = ParseSceneGraph(gltf); 

        cgltf_free(gltf);

        return scene;
    }

    void GLTFConverter::ParseAnimations(cgltf_data* gltf, ParsedScene& scene)
    {
        ParseRigs(gltf, scene);
        ParseClips(gltf, scene);
    }
}