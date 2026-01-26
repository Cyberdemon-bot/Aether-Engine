#include "Aether/Resources/Mesh.h"


#include "aepch.h"
#include "Aether/Resources/Mesh.h"

namespace Aether {
    
    Mesh::Mesh(const MeshSpec& spec)
        : m_SubMeshes(spec.Submeshes)
        , m_VertexCount(spec.Streams[0].VertexCount)
        , m_IndexCount(spec.IndexCount)
    {
        AE_CORE_ASSERT(!spec.Streams.empty(), "Mesh require at least 1 vbo in streams!");
        AE_CORE_ASSERT(spec.IndexData, "Index data cannot be null!");

        m_VertexArray = VertexArray::Create();
        auto ibo = IndexBuffer::Create((uint32_t*)spec.IndexData, spec.IndexCount);
        m_VertexArray->SetIndexBuffer(ibo);

        m_VertexCount = spec.Streams[0].VertexCount;

        for(const auto& vbuffer : spec.Streams)
        {
            AE_CORE_ASSERT(vbuffer.VertexCount == m_VertexCount, "vbuffer's size missmatch in stream!");

            uint32_t stride = vbuffer.Layout.GetStride();
            uint32_t byteSize = vbuffer.VertexCount * stride;

            auto vbo = VertexBuffer::Create((float*)vbuffer.Data, byteSize);
            vbo->SetLayout(vbuffer.Layout);
            m_VertexArray->AddVertexBuffer(vbo);
        }
        // Create default submesh if none provided
        if (m_SubMeshes.empty())
        {
            SubMesh defaultSubMesh;
            defaultSubMesh.BaseVertex = 0;
            defaultSubMesh.BaseIndex = 0;
            defaultSubMesh.VertexCount = m_VertexCount;
            defaultSubMesh.IndexCount = spec.IndexCount;
            defaultSubMesh.NodeName = "Default";
            defaultSubMesh.LocalTransform = glm::mat4(1.0f);
            
            m_SubMeshes.push_back(defaultSubMesh);
        }

        // Calculate bounds from vertex data
        CalculateBounds(spec.Streams[0].Data, m_VertexCount, spec.Streams[0].Layout);
    }

    void Mesh::CalculateBounds(const void* vertexData, uint32_t vertexCount, const BufferLayout& layout)
    {
        const float* verts = static_cast<const float*>(vertexData);
        uint32_t stride = layout.GetStride() / sizeof(float);
        
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
        GetMeshes().reserve(128);
        AE_CORE_INFO("MeshLibrary initialized");
    }

    void MeshLibrary::Shutdown()
    {
        
         GetMeshes().clear();
    }

    Ref<Mesh> MeshLibrary::Load(MeshSpec spec, UUID id)
    {
        auto& meshes = GetMeshes();
        if (meshes.find(id) != meshes.end())
            return meshes[id];

        auto mesh = CreateRef<Mesh>(spec);
        meshes[id] = mesh;
        return mesh;
    }

    Ref<Mesh> MeshLibrary::Get(UUID id)
    {
        auto& meshes = GetMeshes();
        if (meshes.find(id) != meshes.end())
            return meshes[id];
        return nullptr;
    }

    bool MeshLibrary::Exists(UUID id)
    {
        auto& meshes = GetMeshes();
        return meshes.find(id) != meshes.end();
    }

    std::unordered_map<UUID, Ref<Mesh>>& MeshLibrary::GetMeshes()
    {
        static std::unordered_map<UUID, Ref<Mesh>> s_Meshes;
        return s_Meshes;
    }
}