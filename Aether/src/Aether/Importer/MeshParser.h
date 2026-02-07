#pragma once

#include "Aether/Core/Base.h"
#include <vector>
#include <string>
#include <glm/glm.hpp>
    
namespace Aether {
    
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

    struct ParsedMeshInfo
    {
        std::vector<MeshCreateInfo> meshesInfo;
    };

    class MeshParser
    {
    public:
        virtual ~MeshParser() = default;
        virtual Ref<ParsedMeshInfo> Parsing(void* data) = 0;
        static Ref<MeshParser> Create();
    };
}