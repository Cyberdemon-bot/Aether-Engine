#pragma once 
#include "aepch.h"

namespace Aether {
    struct Vertex
    {
        glm::vec3 Position, Normal, Tangent;
        glm::vec2 TexCoord;

        std::array<int, 4> BoneIDs = {-1, -1, -1, -1};
        std::array<float, 4> Weights = {0.0f, 0.0f, 0.0f, 0.0f};

        float orientation; //to calc bitangent

        void AddBone(int id, float weight)
        {
            for(int i = 0; i < 4; i++)
            {
                if(BoneIDs[i] < 0)
                {
                    BoneIDs[i] = id;
                    Weights[i] = weight;
                    return;
                }
            }
        }
    };

    struct MeshData
    {
        std::string Name;
        std::vector<Vertex> Vertices;
        std::vector<uint32_t> Indices;

        glm::vec3 MinBounds{ 0.0f };
        glm::vec3 MaxBounds{ 0.0f };
    };
    
    class Mesh
    {
    public:
        Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name = "New Mesh");

        const MeshData& GetData() const { return m_Data; }
        MeshData& GetData() { return m_Data; }

        void ComputeBoundingBox();
    private:
        MeshData m_Data;
    };
}