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
        Handle<Asset> skeleton;
        std::vector<Handle<Asset>> clips;
    };

    struct ParsedScene
    {
        std::string FilePath;
        std::vector<ImageCreateInfo> Images;
        std::vector<MaterialCreateInfo> Materials;
        std::vector<MeshCreateInfo> Meshes;
        std::vector<SkeletonCreateInfo> Skeletons;      
        std::vector<ClipCreateInfo> Clips;
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

    class AETHER_API GLBAssembler 
    {
    public:
        enum class API {
            None = 0, Cgltf = 1
        };

    public:
        virtual ~GLBAssembler() = default;
		virtual  Ref<ParsedScene> Import(FileData data) = 0;
        
        RegisteredScene Upload(const Ref<ParsedScene>& sceneData);
        static API GetAPI() { return s_API; }
        static Scope<GLBAssembler> Create();

    public: 
        Ref<MeshParser> m_MeshParser;
        Ref<MaterialParser> m_MaterialParser;
        Ref<AnimationParser> m_AnimationParser;
        Ref<SceneGraphParser> m_SceneParser;
    private:
        static API s_API;
    };
}