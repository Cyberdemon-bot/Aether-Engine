#pragma once
#include "Aether/Resources/Mesh.h"
#include "Aether/Resources/Texture.h"
#include "Aether/Resources/Material.h"
#include "Aether/Animation/AnimationClip.h"
#include "Aether/Animation/SkeletalAnimator.h"
#include "Aether/Core/UUID.h"
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace Aether {
    struct SkeletonCreateInfo
    {
        std::string DebugName;
        std::vector<int32_t> parentIndices;
        std::vector<glm::mat4> inverseBindMatrices;
        std::vector<glm::mat4> localBindPose;
        std::vector<std::string> boneNames; 
    };

    struct ClipCreateInfo
    {
        std::string DebugName;
        Clip clip;
    };

    struct TextureCreateInfo
    {
        std::string DebugName;
        TextureSpec Spec;
        std::vector<uint8_t> RawData;
    };

    struct MaterialCreateInfo
    {
        std::string DebugName;
        glm::vec4 AlbedoColor = {1.0f, 1.0f, 1.0f, 1.0f};
        float Metallic = 0.0f;
        float Roughness = 1.0f;
        
        int AlbedoMapIdx = -1;
        int NormalMapIdx = -1;
        int MetallicRoughnessMapIdx = -1;
    };

    struct SubMeshCreateInfo
    {
        std::string NodeName;
        uint32_t VertexCount;
        uint32_t IndexCount;
        uint32_t BaseVertex;
        uint32_t BaseIndex;
        
        glm::vec3 BoundsMin;
        glm::vec3 BoundsMax;
        
        int MaterialIdx = -1;
    };

    struct MeshCreateInfo
    {
        std::string DebugName;
        std::vector<float> Positions;
        std::vector<float> Normals;
        std::vector<float> Tangents;
        std::vector<float> TexCoords;
        std::vector<uint32_t> Indices;

        std::vector<float> Weights;
        std::vector<uint32_t> Joints;

        std::vector<SubMeshCreateInfo> SubMeshes;

        uint32_t totalVertices = 0;
        uint32_t totalIndices = 0;
    };

    struct SceneLoadResult
    {
        std::string FilePath;
        std::vector<TextureCreateInfo> Textures;
        std::vector<MaterialCreateInfo> Materials;
        std::vector<MeshCreateInfo> Meshes;
        std::vector<SkeletonCreateInfo> Skeletons;
        std::vector<ClipCreateInfo> Clips;
    };

    struct SceneResult
    {
        std::vector<UUID> texIDs;
        std::vector<UUID> matIDs;
        std::vector<UUID> meshIDs;
        std::vector<UUID> skeletonIDs;
        std::vector<UUID> clipIDs;
    };
    class AETHER_API SceneLoader
    {
    public:
        static SceneLoadResult Parsing(const std::string& path);
        static SceneResult UploadScene(const SceneLoadResult& sceneData, UUID shaderID);
    };
}