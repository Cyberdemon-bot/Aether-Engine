#include "Platform/Cgltf/GLTF_ImporterAPI.h"
#include "aepch.h"
#include <cgltf.h>

namespace Aether {
    Ref<ParsedScene> GLTF_ImporterAPI::Import(const std::string& path)
    {
        Ref<ParsedScene> SceneData = CreateRef<ParsedScene>();
        SceneData->FilePath = path;

        cgltf_options options = {};
        cgltf_data* gltf = nullptr;
        cgltf_result result = cgltf_parse_file(&options, path.c_str(), &gltf);
        
        if (result != cgltf_result_success)
        {
            AE_CORE_ERROR("Failed to parse GLTF file: {0}", path);
            return SceneData;
        }
        
        result = cgltf_load_buffers(&options, gltf, path.c_str());
        if (result != cgltf_result_success)
        {
            AE_CORE_ERROR("Failed to load GLTF buffers: {0}", path);
            cgltf_free(gltf);
            return SceneData;
        }

        void* data = static_cast<void*>(gltf);
        auto anim = m_AnimationParser->ParseRigAnim(data);
        SceneData->Meshes = std::move(m_MeshParser->Parsing(data)->meshesInfo);
        SceneData->Materials = std::move(m_MaterialParser->Parsing(data)->matsInfo);
        SceneData->Textures = std::move(m_MaterialParser->Parsing(data)->texsInfo);
        SceneData->Rigs = std::move(anim->rigs);
        SceneData->Clips = std::move(anim->clips);
        SceneData->RigMap = std::move(anim->rig_map);
        SceneData->Hierarchy = m_SceneParser->Parsing(data);

        AE_CORE_INFO("Parsed " + path);
        return SceneData;
    }
}