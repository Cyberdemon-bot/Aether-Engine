#include "aepch.h"
#include "Aether/Assets/Mesh.h"
#include "Aether/Renderer/VertexArray.h"
#include "Aether/Renderer/ResourceManager.h"

namespace Aether {

    Mesh::Mesh(const MeshSpec& spec)
        : m_SubMeshes(spec.Submeshes)
    {
        AE_CORE_ASSERT(spec.StreamData, "Mesh require at least 1 vbo in streams!");
        AE_CORE_ASSERT(spec.IndexData, "Index data cannot be null!");

        m_VertexArray = ResourceManager::CreateResource<VertexArray>();
        auto* vao = ResourceManager::GetResource<VertexArray>(m_VertexArray);

        m_IndexBuffer = ResourceManager::CreateResource<IndexBuffer>((uint32_t*)spec.IndexData, spec.IndexCount);
        vao->SetIndexBuffer(ResourceManager::GetResource<IndexBuffer>(m_IndexBuffer));

        uint32_t vertex_cnt = spec.StreamData[0].VertexCount;

        for (int i = 0; i < spec.StreamCount; i++)
        {
            const auto& vbuffer = spec.StreamData[i];
            AE_CORE_ASSERT(vbuffer.VertexCount == vertex_cnt, "vbuffer's size mismatch in stream!");

            uint32_t stride = vbuffer.Layout.GetStride();
            uint32_t byteSize = vbuffer.VertexCount * stride;

            Handle<Resource> vboHandle = ResourceManager::CreateResource<VertexBuffer>((float*)vbuffer.Data, byteSize);
            auto* vbo = ResourceManager::GetResource<VertexBuffer>(vboHandle);
            vbo->SetLayout(vbuffer.Layout);
            vao->AddVertexBuffer(vbo);
            m_VertexBuffers.push_back(vboHandle);
        }

        if (m_SubMeshes.empty())
        {
            SubMesh defaultSubMesh;
            defaultSubMesh.BaseVertex  = 0;
            defaultSubMesh.BaseIndex   = 0;
            defaultSubMesh.VertexCount = vertex_cnt;
            defaultSubMesh.IndexCount  = spec.IndexCount;
            m_SubMeshes.push_back(defaultSubMesh);
        }

        if (spec.CalculateBoundsFunc)
        {
            auto [tempMin, tempMax] = spec.CalculateBoundsFunc(spec);
            m_BoundsMin = tempMin;
            m_BoundsMax = tempMax;
            m_BoundsCenter = (m_BoundsMin + m_BoundsMax) * 0.5f;
            m_BoundsExtents = (m_BoundsMax - m_BoundsMin) * 0.5f;
        }

        if (spec.CalculateAnimatedBoundsFunc)
        {
            auto [tempAnimMin, tempAnimMax] = spec.CalculateAnimatedBoundsFunc(spec);
            m_AnimatedBoundsMin = tempAnimMin;
            m_AnimatedBoundsMax = tempAnimMax;
            m_HasAnimatedBounds = true;
        }
        else m_HasAnimatedBounds = false;
    }

    Mesh::~Mesh()
    {
        for (auto& vbo : m_VertexBuffers)
            ResourceManager::Unload(vbo);

        ResourceManager::Unload(m_IndexBuffer);
        ResourceManager::Unload(m_VertexArray);
    }
}