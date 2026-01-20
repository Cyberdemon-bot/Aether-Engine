#include "Aether/Resources/Mesh.h"

namespace Aether {
    
    Mesh::Mesh(const void* vertexData, uint32_t vertexCount, 
               const uint32_t* indexData, uint32_t indexCount,
               const BufferLayout& layout, const std::vector<SubMesh>& submeshes)
        : m_Layout(layout)
        , m_VertexCount(vertexCount)
        , m_IndexCount(indexCount)
        , m_SubMeshes(submeshes)
    {
        AE_CORE_ASSERT(vertexData, "Vertex data is null!");
        AE_CORE_ASSERT(indexData, "Index data is null!");
        AE_CORE_ASSERT(vertexCount > 0, "Vertex count is zero!");
        AE_CORE_ASSERT(indexCount > 0, "Index count is zero!");

        uint32_t vertexBufferSize = vertexCount * m_Layout.GetStride();

        m_VertexBuffer = VertexBuffer::Create((float*)vertexData, vertexBufferSize);
        m_VertexBuffer->SetLayout(m_Layout);

        m_IndexBuffer = IndexBuffer::Create((uint32_t*)indexData, indexCount);

        m_VertexArray = VertexArray::Create();
        m_VertexArray->AddVertexBuffer(m_VertexBuffer);
        m_VertexArray->SetIndexBuffer(m_IndexBuffer);

        if (m_SubMeshes.empty())
        {
            SubMesh defaultSubMesh;
            defaultSubMesh.BaseVertex = 0;
            defaultSubMesh.BaseIndex = 0;
            defaultSubMesh.VertexCount = vertexCount;
            defaultSubMesh.IndexCount = indexCount;
            defaultSubMesh.NodeName = "Default";
            defaultSubMesh.LocalTransform = glm::mat4(1.0f);
            
            m_SubMeshes.push_back(defaultSubMesh);
        }
    }

}