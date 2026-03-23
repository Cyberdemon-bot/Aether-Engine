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
            m_SubMeshes.push_back(defaultSubMesh);
        }

        // Calculate bounds from first vertex stream
        CalculateBounds(spec.Streams[0].Data, m_VertexCount, spec.Streams[0].Layout);
        if (!spec.RigPoseMats.empty())
        {
            m_HasAnimatedBounds = true;
            CalculateAnimatedBounds(spec.Streams[0].Data, spec.Streams[0].Layout, 
                                    spec.Streams[4].Data, spec.Streams[4].Layout, 
                                    spec.Streams[5].Data, spec.Streams[5].Layout, 
                                    m_VertexCount, spec.RigPoseMats);
        }
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

    void Mesh::UploadMesh()
    {
        auto* vao = ResourceManager::GetResource<VertexArray>(m_VertexArray);
        vao->Bind();
    }

    void Mesh::CalculateAnimatedBounds(
        const void* positions, const BufferLayout& posLayout,
        const void* joints,    const BufferLayout& jointLayout,
        const void* weights,   const BufferLayout& weightLayout,
        uint32_t vertexCount,
        const std::vector<glm::mat4>& poseMats)
    {
        if (!positions || !joints || !weights || vertexCount == 0 || poseMats.empty()) return;

        uint32_t posStride    = posLayout.GetStride();
        uint32_t jointStride  = jointLayout.GetStride();
        uint32_t weightStride = weightLayout.GetStride();

        const uint8_t* posBase    = static_cast<const uint8_t*>(positions);
        const uint8_t* jointBase  = static_cast<const uint8_t*>(joints);
        const uint8_t* weightBase = static_cast<const uint8_t*>(weights);

        glm::vec3 boundsMin( FLT_MAX);
        glm::vec3 boundsMax(-FLT_MAX);

        for (uint32_t i = 0; i < vertexCount; i++)
        {
            glm::vec3  pos = *reinterpret_cast<const glm::vec3*> (posBase    + i * posStride);
            glm::uvec4 b   = *reinterpret_cast<const glm::uvec4*>(jointBase  + i * jointStride);
            glm::vec4  w   = *reinterpret_cast<const glm::vec4*> (weightBase + i * weightStride);

            glm::vec4 skinnedPos =
                poseMats[b.x] * glm::vec4(pos, 1.0f) * w.x +
                poseMats[b.y] * glm::vec4(pos, 1.0f) * w.y +
                poseMats[b.z] * glm::vec4(pos, 1.0f) * w.z +
                poseMats[b.w] * glm::vec4(pos, 1.0f) * w.w;

            boundsMin = glm::min(boundsMin, glm::vec3(skinnedPos));
            boundsMax = glm::max(boundsMax, glm::vec3(skinnedPos));
        }

        m_AnimatedBoundsMin = boundsMin;
        m_AnimatedBoundsMax = boundsMax;
    }
}