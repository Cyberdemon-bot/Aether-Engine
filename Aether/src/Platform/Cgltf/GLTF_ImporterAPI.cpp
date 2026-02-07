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
        SceneData.Meshes = m_MeshParser->Parsing(data)->meshesInfo;
        SceneData.Materials = m_MaterialParser->Parsing(data)->matsInfo;
        SceneData.Textures = m_MaterialParser->Parsing(data)->texsInfo;
        SceneData.Skeletons = m_AnimationParser->Parsing(data)->skeletons;
        SceneData.Clips = m_AnimationParser->Parsing(data)->clips;

        return SceneData;
    }
}