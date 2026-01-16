#include "Aether/Renderer/Mesh.h"

namespace Aether {
    Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, const std::string& name)
    {
        m_Data.Vertices = vertices;
        m_Data.Indices = indices;
        m_Data.Name = name;
        ComputeBoundingBox();
    }

    void Mesh::ComputeBoundingBox() 
    {
        if (m_Data.Vertices.empty()) return;
        m_Data.MinBounds = m_Data.MaxBounds = m_Data.Vertices[0].Position;
        for (const auto& v : m_Data.Vertices) 
        {
            m_Data.MinBounds = glm::min(m_Data.MinBounds, v.Position);
            m_Data.MaxBounds = glm::max(m_Data.MaxBounds, v.Position);
        }
    }
}