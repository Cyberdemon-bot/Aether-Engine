#pragma once
#include "aepch.h"
#include "Aether/Core/UUID.h"
#include "Aether/Renderer/VertexArray.h"
#include "Aether/Renderer/Buffer.h"

namespace Aether {
    struct SubMesh
    {
        uint32_t BaseVertex = 0;
        uint32_t BaseIndex = 0;
        uint32_t VertexCount = 0;
        uint32_t IndexCount = 0;

        glm::vec3 BoundsMin = glm::vec3(0.0f);
        glm::vec3 BoundsMax = glm::vec3(0.0f);

        std::string NodeName;
        glm::mat4 LocalTransform = glm::mat4(1.0f);

        UUID MaterialID = 0;
    };

    class MeshLayout 
    {
    public:
        static BufferLayout PBR() {
            return {
                { "a_Position",  ShaderDataType::Float3 },
                { "a_Normal",    ShaderDataType::Float3 },
                { "a_Tangent",   ShaderDataType::Float3 },
                { "a_Bitangent", ShaderDataType::Float3 },
                { "a_TexCoord",  ShaderDataType::Float2 }
            };
        }

        static BufferLayout Phong() {
            return {
                { "a_Position", ShaderDataType::Float3 },
                { "a_Normal",   ShaderDataType::Float3 },
                { "a_TexCoord", ShaderDataType::Float2 }
            };
        }

        static BufferLayout PBRSkinned() {
            return {
                { "a_Position",    ShaderDataType::Float3 },
                { "a_Normal",      ShaderDataType::Float3 },
                { "a_Tangent",     ShaderDataType::Float3 },
                { "a_Bitangent",   ShaderDataType::Float3 },
                { "a_TexCoord",    ShaderDataType::Float2 },
                { "a_BoneIDs",     ShaderDataType::Int4 },
                { "a_BoneWeights", ShaderDataType::Float4 }
            };
        }

        static BufferLayout Quad() {
            return {
                { "a_Position", Aether::ShaderDataType::Float2 },
                { "a_TexCoord", Aether::ShaderDataType::Float2 }
            };
        }

        static BufferLayout Vertex() {
            return {
                { "a_Position", Aether::ShaderDataType::Float3 }
            };
        }
    };

    struct MeshSpec
    {
        const void* VertexData = nullptr;
        uint32_t VertexCount = 0;
        const uint32_t* IndexData = nullptr;
        uint32_t IndexCount = 0;
        BufferLayout Layout;
        std::vector<SubMesh> Submeshes = {};
    };

    class AETHER_API Mesh 
    {
    public:
        Mesh(const MeshSpec& spec);
        
        Ref<VertexArray> GetVertexArray() const { return m_VertexArray; }
        const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
        const BufferLayout& GetLayout() const { return m_Layout; }
        
        uint32_t GetVertexCount() const { return m_VertexCount; }
        uint32_t GetIndexCount() const { return m_IndexCount; }

        const glm::vec3& GetBoundsMin() const { return m_BoundsMin; }
        const glm::vec3& GetBoundsMax() const { return m_BoundsMax; }
        glm::vec3 GetBoundsCenter() const { return (m_BoundsMin + m_BoundsMax) * 0.5f; }
        glm::vec3 GetBoundsExtents() const { return (m_BoundsMax - m_BoundsMin) * 0.5f; }

    private:
        Ref<VertexArray> m_VertexArray;
        Ref<VertexBuffer> m_VertexBuffer;
        Ref<IndexBuffer> m_IndexBuffer;

        BufferLayout m_Layout;
        std::vector<SubMesh> m_SubMeshes;
        
        uint32_t m_VertexCount = 0;
        uint32_t m_IndexCount = 0;

        glm::vec3 m_BoundsMin = glm::vec3(0.0f);
        glm::vec3 m_BoundsMax = glm::vec3(0.0f);

        void CalculateBounds(const void* vertexData, uint32_t vertexCount);
    };

    class AETHER_API MeshLibrary
    {
    public:
        static void Init();
        static void Shutdown();

        static Ref<Mesh> Load(MeshSpec spec, UUID id);
        static Ref<Mesh> Get(UUID id);
        static bool Exists(UUID id);

    private:
        static std::unordered_map<UUID, Ref<Mesh>> s_Meshes;
    };
}