#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Renderer/Buffer.h"
#include "Aether/Assets/Asset.h"
#include "Aether/Container/Handle.h"

#include <vector>
#include <unordered_map>

namespace Aether {

    class Resource;
    struct SubMesh
    {
        uint32_t BaseVertex = 0;
        uint32_t BaseIndex = 0;
        uint32_t VertexCount = 0;
        uint32_t IndexCount = 0;
        glm::vec3 BoundsMin = glm::vec3(0.0f);
        glm::vec3 BoundsMax = glm::vec3(0.0f);
        int MaterialIdx = -1;
    };

    class MeshLayout 
    {
    public:
        static BufferLayout PBR() 
        {
            return {
                { "a_Position",  ShaderDataType::Float3 },
                { "a_Normal",    ShaderDataType::Float3 },
                { "a_Tangent",   ShaderDataType::Float4 },
                { "a_TexCoord",  ShaderDataType::Float2 },
            };
        }

        static BufferLayout Phong() 
        {
            return {
                { "a_Position", ShaderDataType::Float3 },
                { "a_Normal",   ShaderDataType::Float3 },
                { "a_TexCoord", ShaderDataType::Float2 }
            };
        }

        static BufferLayout PBRSkinned() 
        {
            return {
                { "a_Position",  ShaderDataType::Float3 },
                { "a_Normal",    ShaderDataType::Float3 },
                { "a_Tangent",   ShaderDataType::Float4 },
                { "a_TexCoord",  ShaderDataType::Float2 },
                { "a_Joints",    ShaderDataType::Uint4  },
                { "a_Weights",   ShaderDataType::Float4 }
            };
        }

        static BufferLayout Quad() 
        {
            return {
                { "a_Position", ShaderDataType::Float2 },
                { "a_TexCoord", ShaderDataType::Float2 }
            };
        }

        static BufferLayout Vertex() 
        {
            return {
                { "a_Position", ShaderDataType::Float3 }
            };
        }
    };

    enum class VertexLayoutLocation : uint32_t 
    {
        Position = 0,
        Normal = 1,
        TexCoord = 2,
        Tangent = 3,
        Color = 4,
        Joints = 5,    
        InstanceStart = 6     
    };

    struct VertexStream
    {
        const void* Data = nullptr;
        uint32_t VertexCount = 0;
        BufferLayout Layout = MeshLayout::Vertex();
    };

    struct MeshSpec
    {
        const VertexStream* StreamData = nullptr;
        uint32_t StreamCount = 0;
        const uint32_t* IndexData = nullptr;
        uint32_t IndexCount = 0;
        std::vector<SubMesh> Submeshes = {};
        std::vector<glm::mat4> RigPoseMats = {};
    };

    struct Mesh : public Asset
    {
        Mesh(const MeshSpec& spec);
        ~Mesh();

        Handle<Resource> m_VertexArray;
        Handle<Resource> m_IndexBuffer;
        std::vector<Handle<Resource>> m_VertexBuffers;
        std::vector<SubMesh> m_SubMeshes;

        glm::vec3 m_BoundsMin = glm::vec3(0.0f);
        glm::vec3 m_BoundsMax = glm::vec3(0.0f);
        glm::vec3 m_BoundsCenter = glm::vec3(0.0f);
        glm::vec3 m_BoundsExtents = glm::vec3(0.0f);
        glm::vec3 m_AnimatedBoundsMin = glm::vec3(0.0f);
        glm::vec3 m_AnimatedBoundsMax = glm::vec3(0.0f);

        bool m_HasAnimatedBounds = false;
        bool m_HasInstanceBuffer = false;
    };
}