#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <span>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include "Aether/Assets/CreateInfoList.h"
#include "Aether/FileSystem/FileData.h"

struct cgltf_data;
struct cgltf_node;

namespace Aether {

    struct Vertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec4 Tangent;
        glm::vec2 TexCoord;
    };

    struct SkinnedVertex
    {
        glm::vec3 Position;
        glm::vec3 Normal;
        glm::vec4 Tangent;
        glm::vec2 TexCoord;
        glm::uvec4 Joints;
        glm::vec4 Weights;
    };


    struct Node
    {
        std::string name;
        glm::vec3 translation = glm::vec3(0.0f);
        glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale = glm::vec3(1.0f);
        int meshIdx = -1;
        int animatorIdx = -1;
        std::vector<int> children;
    };

    struct SceneHierarchy
    {
        std::vector<Node> nodes;
        std::vector<int> roots;
    };

    class GLTFResult : public CreateInfoList
    {
    public:
        GLTFResult() = default;
    private:
        std::vector<std::vector<uint8_t>> ImagePixels;
        std::vector<std::vector<UUID>> SheetData;
        std::vector<std::vector<uint8_t>> MeshVertexBytes;
        std::vector<std::vector<uint32_t>> MeshIndexData;
        std::vector<std::vector<VertexStream>> MeshStreamData;
        std::vector<std::vector<Submesh>> MeshSubmeshData;

        std::vector<AImageCreateInfo> tempImages;
        std::vector<AMaterialCreateInfo> tempMaterials;
        std::vector<ASkeletonCreateInfo> tempSkels;

        Ref<SceneHierarchy> Hierarchy;

        friend class GLTFConverter;
    };

    class GLTFConverter
    {
    public:
        Ref<CreateInfoList> Import(const FileData& data, Ref<SceneHierarchy>& hierarchy);
        Ref<CreateInfoList> Import(const FileData& data);
    private:
        void ParseMaterials(cgltf_data* gltf, GLTFResult& result);
        void ParseRigs(cgltf_data* gltf, GLTFResult& result);
        void ParseClips(cgltf_data* gltf, GLTFResult& result);
        void ParseAnimations(cgltf_data* gltf, GLTFResult& result);
        void ParseMeshes(cgltf_data* gltf, GLTFResult& result);

        Ref<SceneHierarchy> ParseSceneGraph(cgltf_data* gltf);
        void ParseNode(cgltf_data* gltf, cgltf_node* node, const Ref<SceneHierarchy>& out,
                       int parentNodeIdx, const std::unordered_set<cgltf_node*>& jointNodes);
    };
}