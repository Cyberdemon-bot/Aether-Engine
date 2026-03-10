#include "aepch.h"
#include "Aether/Renderer/VertexArray.h"
#include "Aether/Assets/Mesh.h"

namespace Aether {

    Mesh::Mesh(const MeshSpec& spec)
        : m_SubMeshes(spec.Submeshes)
        , m_VertexCount(spec.Streams[0].VertexCount)
        , m_IndexCount(spec.IndexCount)
    {
        AE_CORE_ASSERT(!spec.Streams.empty(), "Mesh require at least 1 vbo in streams!");
        AE_CORE_ASSERT(spec.IndexData, "Index data cannot be null!");

        m_VertexArray = ResourceManager::CreateResource<VertexArray>();
        auto* vao = ResourceManager::GetResource<VertexArray>(m_VertexArray);

        m_IndexBuffer = ResourceManager::CreateResource<IndexBuffer>((uint32_t*)spec.IndexData, spec.IndexCount);
        vao->SetIndexBuffer(ResourceManager::GetResource<IndexBuffer>(m_IndexBuffer));

        m_VertexCount = spec.Streams[0].VertexCount;

        for (const auto& vbuffer : spec.Streams)
        {
            AE_CORE_ASSERT(vbuffer.VertexCount == m_VertexCount, "vbuffer's size mismatch in stream!");

            uint32_t stride   = vbuffer.Layout.GetStride();
            uint32_t byteSize = vbuffer.VertexCount * stride;

            ResourceHandle vboHandle = ResourceManager::CreateResource<VertexBuffer>((float*)vbuffer.Data, byteSize);
            auto* vbo = ResourceManager::GetResource<VertexBuffer>(vboHandle);
            vbo->SetLayout(vbuffer.Layout);
            vao->AddVertexBuffer(vbo);
            m_VertexBuffers.push_back(vboHandle);
        }

        // Create default submesh if none provided
        if (m_SubMeshes.empty())
        {
            SubMesh defaultSubMesh;
            defaultSubMesh.BaseVertex  = 0;
            defaultSubMesh.BaseIndex   = 0;
            defaultSubMesh.VertexCount = m_VertexCount;
            defaultSubMesh.IndexCount  = spec.IndexCount;
            defaultSubMesh.NodeName    = "Default";
            m_SubMeshes.push_back(defaultSubMesh);
        }

        // Calculate bounds from first vertex stream
        CalculateBounds(spec.Streams[0].Data, m_VertexCount, spec.Streams[0].Layout);
    }

    Mesh::~Mesh()
    {
        for (auto& vbo : m_VertexBuffers)
            ResourceManager::Unload(vbo);

        ResourceManager::Unload(m_IndexBuffer);
        ResourceManager::Unload(m_VertexArray);
    }

    void Mesh::CalculateBounds(const void* vertexData, uint32_t vertexCount, const BufferLayout& layout)
    {
        const float* verts = static_cast<const float*>(vertexData);
        uint32_t stride = layout.GetStride() / sizeof(float);

        m_BoundsMin = glm::vec3( FLT_MAX);
        m_BoundsMax = glm::vec3(-FLT_MAX);

        // Assumes position is the first 3 floats in each vertex
        for (uint32_t i = 0; i < vertexCount; i++)
        {
            glm::vec3 pos(verts[i * stride], verts[i * stride + 1], verts[i * stride + 2]);
            m_BoundsMin = glm::min(m_BoundsMin, pos);
            m_BoundsMax = glm::max(m_BoundsMax, pos);
        }
    }
}