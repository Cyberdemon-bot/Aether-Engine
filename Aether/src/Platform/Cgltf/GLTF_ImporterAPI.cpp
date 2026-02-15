#include "Platform/Cgltf/GLTF_ImporterAPI.h"
#include "aepch.h"
#include <cgltf.h>

namespace Aether {
    ParsedScene GLTF_ImporterAPI::Import(const std::string& path)
    {
        ParsedScene SceneData;
        SceneData.FilePath = path;

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
        SceneData.Meshes = m_MeshParser->Parsing(data)->meshesInfo;
        SceneData.Materials = m_MaterialParser->Parsing(data)->matsInfo;
        SceneData.Textures = m_MaterialParser->Parsing(data)->texsInfo;
        SceneData.Rigs = anim->rigs;
        SceneData.Clips = anim->clips;
        SceneData.RigMap = anim->rig_map;

        AE_CORE_INFO("Parsed " + path);
        return SceneData;
    }
}