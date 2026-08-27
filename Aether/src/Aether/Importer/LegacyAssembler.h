#pragma once

#include <string>
#include <glm/glm.hpp>
#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include "Aether/Importer/MaterialParser.h"
#include "Aether/Importer/MeshParser.h"
#include "Aether/Importer/AnimationParser.h"
#include "Aether/Importer/SceneGraphParser.h"
#include "Aether/FileSystem/FileData.h"

namespace Aether {

    struct Asset;

    struct Animator
    {
        UUID skeleton;
        std::vector<UUID> clips;
    };

    struct ParsedScene
    {
        std::string FilePath;
        std::vector<LImageCreateInfo> Images;
        std::vector<LMaterialCreateInfo> Materials;
        std::vector<LMeshCreateInfo> Meshes;
        std::vector<LSkeletonCreateInfo> Skeletons;      
        std::vector<LClipCreateInfo> Clips;
        Ref<SceneHierarchy> Hierarchy;
    };

    struct RegisteredScene
    {
        std::vector<UUID> meshIDs;
        std::vector<UUID> matIDs;
        std::vector<UUID> sheetIDs;
        std::vector<Animator> animators;
        Ref<SceneHierarchy> hierarchy;
    };

    class AETHER_API LegacyAssembler 
    {
    public:
        enum class API {
            None = 0, Cgltf = 1
        };

    public:
        virtual ~LegacyAssembler() = default;
		virtual  Ref<ParsedScene> Import(FileData data) = 0;
        
        RegisteredScene Upload(const Ref<ParsedScene>& sceneData);
        static API GetAPI() { return s_API; }
        static Scope<LegacyAssembler> Create();

    public: 
        Ref<MeshParser> m_MeshParser;
        Ref<MaterialParser> m_MaterialParser;
        Ref<AnimationParser> m_AnimationParser;
        Ref<SceneGraphParser> m_SceneParser;
    private:
        static API s_API;
    };
}