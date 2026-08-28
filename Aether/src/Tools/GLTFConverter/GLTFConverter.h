#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <span>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include "Aether/FileSystem/FileData.h"
#include "Aether/Assets/RegisterInfo.h"  

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

    struct ParsedScene
    {
        std::string FilePath;

        std::vector<AImageCreateInfo> Images;
        std::vector<AMaterialCreateInfo> Materials;
        std::vector<ASkeletonCreateInfo> Skeletons;
        std::vector<AClipCreateInfo> Clips;
        std::vector<AMeshCreateInfo> Meshes;

        Ref<SceneHierarchy> Hierarchy;

        std::vector<std::vector<uint8_t>> ImagePixels;    
        std::vector<std::vector<uint8_t>> MeshVertexBytes; 
        std::vector<std::vector<uint32_t>> MeshIndexData;   
        std::vector<std::vector<VertexStream>> MeshStreamData;  
        std::vector<std::vector<SubMesh>> MeshSubmeshData;
    };

    class GLTFConverter
    {
    public:
        ParsedScene Import(const FileData& data);

    private:
        void ParseMaterials(cgltf_data* gltf, ParsedScene& scene);   
        void ParseRigs(cgltf_data* gltf, ParsedScene& scene);        
        void ParseClips(cgltf_data* gltf, ParsedScene& scene);       
        void ParseAnimations(cgltf_data* gltf, ParsedScene& scene);  
        void ParseMeshes(cgltf_data* gltf, ParsedScene& scene);  

        Ref<SceneHierarchy> ParseSceneGraph(cgltf_data* gltf);
        void ParseNode(cgltf_data* gltf, cgltf_node* node, const Ref<SceneHierarchy>& out,
                       int parentNodeIdx, const std::unordered_set<cgltf_node*>& jointNodes);
    };
}