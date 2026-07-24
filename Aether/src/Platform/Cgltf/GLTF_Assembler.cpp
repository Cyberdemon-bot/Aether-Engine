#include "aepch.h"
#include <cgltf.h>
#include "Platform/Cgltf/GLTF_Assembler.h"

namespace Aether {
    Ref<ParsedScene> GLTF_Assembler::Import(FileData data)
    {
        Ref<ParsedScene> SceneData = CreateRef<ParsedScene>();

        cgltf_options options = {};
        cgltf_data* gltf = nullptr;
        cgltf_result result = cgltf_parse(&options, data.bytes, data.size, &gltf);

        if (result != cgltf_result_success)
        {
            AE_CORE_ERROR("Failed to parse GLB data");
            return SceneData;
        }

        result = cgltf_load_buffers(&options, gltf, nullptr);
        if (result != cgltf_result_success)
        {
            AE_CORE_ERROR("Failed to load GLB buffers");
            cgltf_free(gltf);
            return SceneData;
        }

        void* raw = static_cast<void*>(gltf);
        auto anim = m_AnimationParser->ParseRigAnim(raw);
        auto matRes = m_MaterialParser->Parsing(raw);
        auto meshRes = m_MeshParser->Parsing(raw);
        SceneData->Hierarchy = m_SceneParser->Parsing(raw);
        SceneData->Meshes = std::move(meshRes->meshesInfo);
        SceneData->Materials = std::move(matRes->matsInfo);
        SceneData->Images = std::move(matRes->imgsInfo);
        SceneData->Skeletons = std::move(anim->rigs);
        SceneData->Clips = std::move(anim->clips);
        return SceneData;
    }
}