#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/UUID.h"
#include "Aether/Renderer/Buffer.h"
#include "Aether/Assets/AssetManager.h"
#include "Aether/Renderer/ResourceManager.h"

#include <vector>
#include <unordered_map>

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
        int MaterialIdx = -1;
    };

    class MeshLayout 
    {
    public:
        static BufferLayout PBR() {
            return {
                { "a_Position",  ShaderDataType::Float3 },
                { "a_Normal",    ShaderDataType::Float3 },
                { "a_Tangent",   ShaderDataType::Float4 },
                { "a_TexCoord",  ShaderDataType::Float2 },
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
                { "a_Position",  ShaderDataType::Float3 },
                { "a_Normal",    ShaderDataType::Float3 },
                { "a_Tangent",   ShaderDataType::Float4 },
                { "a_TexCoord",  ShaderDataType::Float2 },
                { "a_Joints",    ShaderDataType::Uint4  },
                { "a_Weights",   ShaderDataType::Float4 }
            };
        }

        static BufferLayout Quad() {
            return {
                { "a_Position", ShaderDataType::Float2 },
                { "a_TexCoord", ShaderDataType::Float2 }
            };
        }

        static BufferLayout Vertex() {
            return {
                { "a_Position", ShaderDataType::Float3 }
            };
        }
    };

    struct VertexStream
    {
        const void* Data = nullptr;
        uint32_t VertexCount = 0;
        BufferLayout Layout = MeshLayout::Vertex();
    };

    struct MeshSpec
    {
        std::vector<VertexStream> Streams;
        const uint32_t* IndexData = nullptr;
        uint32_t IndexCount = 0;
        std::vector<SubMesh> Submeshes = {};
        std::vector<glm::mat4> RigPoseMats = {};
    };

    class AETHER_API Mesh : public Asset
    {
    public:
        Mesh(const MeshSpec& spec);
        ~Mesh();

        ResourceHandle GetVertexArray() const { return m_VertexArray; }
        const std::vector<SubMesh>& GetSubMeshes() const { return m_SubMeshes; }
        const BufferLayout& GetLayout() const { return m_Layout; }

        uint32_t GetVertexCount() const { return m_VertexCount; }
        uint32_t GetIndexCount()  const { return m_IndexCount; }

        const glm::vec3& GetBoundsMin()   const { return m_BoundsMin; }
        const glm::vec3& GetBoundsMax()   const { return m_BoundsMax; }
        const glm::vec3& GetAnimatedBoundsMin()   const { return m_AnimatedBoundsMin; }
        const glm::vec3& GetAnimatedBoundsMax()   const { return m_AnimatedBoundsMax; }
        glm::vec3        GetBoundsCenter()  const { return (m_BoundsMin + m_BoundsMax) * 0.5f; }
        glm::vec3        GetBoundsExtents() const { return (m_BoundsMax - m_BoundsMin) * 0.5f; }
        bool HasAnimatedBounds() { return m_HasAnimatedBounds; }

        static Ref<Mesh> Create(const MeshSpec& spec) { return CreateRef<Mesh>(spec); }

    private:
        ResourceHandle m_VertexArray;
        ResourceHandle m_IndexBuffer;
        std::vector<ResourceHandle> m_VertexBuffers;

        BufferLayout m_Layout;
        std::vector<SubMesh> m_SubMeshes;

        uint32_t m_VertexCount = 0;
        uint32_t m_IndexCount  = 0;
        glm::vec3 m_BoundsMin   = glm::vec3(0.0f);
        glm::vec3 m_BoundsMax   = glm::vec3(0.0f);
        glm::vec3 m_AnimatedBoundsMin   = glm::vec3(0.0f);
        glm::vec3 m_AnimatedBoundsMax   = glm::vec3(0.0f);
        bool m_HasAnimatedBounds = false;

        void CalculateBounds(const void* vertexData, uint32_t vertexCount, const BufferLayout& layout);
        void CalculateAnimatedBounds(
            const void* positions, const BufferLayout& posLayout,
            const void* joints,    const BufferLayout& jointLayout,
            const void* weights,   const BufferLayout& weightLayout,
            uint32_t vertexCount,
            const std::vector<glm::mat4>& poseMats);

        static Scope<Mesh> CreateImpl(const MeshSpec& spec) { return CreateScope<Mesh>(spec); }

        static const AssetType GetType() { return AssetType::Mesh; }
        virtual const AssetType GetAssetType() const override { return AssetType::Mesh; }
        friend class AssetManager;
    };
}