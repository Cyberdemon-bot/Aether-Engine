#include "aepch.h"
#include "GLTFConverter.h"
#include "Aether/Core/Assert.h"

#include <cgltf.h>

namespace Aether {

    Ref<CreateInfoList> GLTFConverter::Import(const FileData& data, Ref<SceneHierarchy>& hierarchy)
    {
        Ref<GLTFResult> result = CreateRef<GLTFResult>();

        cgltf_options options = {};
        cgltf_data* gltf = nullptr;
        cgltf_result r = cgltf_parse(&options, data.bytes, data.size, &gltf);

        if (r != cgltf_result_success)
        {
            AE_CORE_ERROR("GLTFConverter: failed to parse glTF/GLB data");
            return result;
        }

        r = cgltf_load_buffers(&options, gltf, nullptr);
        if (r != cgltf_result_success)
        {
            AE_CORE_ERROR("GLTFConverter: failed to load glTF/GLB buffers");
            cgltf_free(gltf);
            return result;
        }

        result->Hierarchy = ParseSceneGraph(gltf);
        hierarchy = result->Hierarchy;

        ParseMaterials(gltf, *result);
        ParseAnimations(gltf, *result);
        ParseMeshes(gltf, *result);

        cgltf_free(gltf);

        return result;
    }

    Ref<CreateInfoList> GLTFConverter::Import(const FileData& data)
    {
        Ref<SceneHierarchy> hierarchy;
        return Import(data, hierarchy);
    }

    void GLTFConverter::ParseAnimations(cgltf_data* gltf, GLTFResult& result)
    {
        ParseRigs(gltf, result);
        ParseClips(gltf, result);
    }
}