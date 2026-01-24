#include "Aether/Resources/Mesh.h"


#include "aepch.h"
#include "Aether/Resources/Mesh.h"

namespace Aether {
    
    Mesh::Mesh(const MeshSpec& spec)
        : m_Layout(spec.Layout)
        , m_VertexCount(spec.VertexCount)
        , m_IndexCount(spec.IndexCount)
        , m_SubMeshes(spec.Submeshes)
    {
        AE_CORE_ASSERT(spec.VertexData, "Vertex data is null!");
        AE_CORE_ASSERT(spec.IndexData, "Index data is null!");
        AE_CORE_ASSERT(spec.VertexCount > 0, "Vertex count is zero!");
        AE_CORE_ASSERT(spec.IndexCount > 0, "Index count is zero!");

        uint32_t vertexBufferSize = spec.VertexCount * m_Layout.GetStride();

        m_VertexBuffer = VertexBuffer::Create((float*)spec.VertexData, vertexBufferSize);
        m_VertexBuffer->SetLayout(m_Layout);

        m_IndexBuffer = IndexBuffer::Create((uint32_t*)spec.IndexData, spec.IndexCount);

        m_VertexArray = VertexArray::Create();
        m_VertexArray->AddVertexBuffer(m_VertexBuffer);
        m_VertexArray->SetIndexBuffer(m_IndexBuffer);

        // Create default submesh if none provided
        if (m_SubMeshes.empty())
        {
            SubMesh defaultSubMesh;
            defaultSubMesh.BaseVertex = 0;
            defaultSubMesh.BaseIndex = 0;
            defaultSubMesh.VertexCount = spec.VertexCount;
            defaultSubMesh.IndexCount = spec.IndexCount;
            defaultSubMesh.NodeName = "Default";
            defaultSubMesh.LocalTransform = glm::mat4(1.0f);
            
            m_SubMeshes.push_back(defaultSubMesh);
        }

        // Calculate bounds from vertex data
        CalculateBounds(spec.VertexData, spec.VertexCount);
    }

    void Mesh::CalculateBounds(const void* vertexData, uint32_t vertexCount)
    {
        const float* verts = static_cast<const float*>(vertexData);
        uint32_t stride = m_Layout.GetStride() / sizeof(float);
        
        m_BoundsMin = glm::vec3(FLT_MAX);
        m_BoundsMax = glm::vec3(-FLT_MAX);
        
        // Assume position is the first 3 floats in each vertex
        for (uint32_t i = 0; i < vertexCount; i++)
        {
            glm::vec3 pos(verts[i * stride], verts[i * stride + 1], verts[i * stride + 2]);
            m_BoundsMin = glm::min(m_BoundsMin, pos);
            m_BoundsMax = glm::max(m_BoundsMax, pos);
        }
    }

    void MeshLibrary::Init()
    {
        AE_CORE_INFO("MeshLibrary initialized");
    }

    void MeshLibrary::Shutdown()
    {
        s_Meshes.clear();
    }

    Ref<Mesh> MeshLibrary::Load(MeshSpec spec, UUID id)
    {
        if (s_Meshes.find(id) != s_Meshes.end())
            return s_Meshes[id];

        auto mesh = CreateRef<Mesh>(spec);
        s_Meshes[id] = mesh;
        return mesh;
    }

    Ref<Mesh> MeshLibrary::Get(UUID id)
    {
        if (s_Meshes.find(id) != s_Meshes.end())
            return s_Meshes[id];
        return nullptr;
    }

    bool MeshLibrary::Exists(UUID id)
    {
        return s_Meshes.find(id) != s_Meshes.end();
    }

    std::unordered_map<UUID, Ref<Mesh>> MeshLibrary::s_Meshes;
}