#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include <vector>
#include <string>
#include <glm/glm.hpp>
    
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
    
    struct SubAMeshCreateInfo
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

    struct LMeshCreateInfo
    {
        UUID AssetID;
        std::string DebugName;
        bool IsSkinned = false;
        
        std::vector<uint8_t> InterleavedVertices;
        std::vector<uint32_t> Indices;
        std::vector<SubAMeshCreateInfo> SubMeshes;

        uint32_t totalVertices = 0;
        uint32_t totalIndices = 0;
    };

    struct ParsedMeshInfo
    {
        std::vector<LMeshCreateInfo> meshesInfo;
    };

    class MeshParser
    {
    public:
        virtual ~MeshParser() = default;
        virtual Ref<ParsedMeshInfo> Parsing(void* data) = 0;
        static Ref<MeshParser> Create();
    };
}