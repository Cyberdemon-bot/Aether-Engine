#pragma once

#include "Aether/Core/Base.h"

namespace Aether {
    enum class ResourceType
    {
       Texture2D, TextureCube, Shader, UniformBuffer,
       VertexBuffer, IndexBuffer, VertexArray, FrameBuffer
    };

    struct ResourceHandle
    {
        uint32_t index = UINT32_MAX;
        uint32_t generation = 0;
        bool IsValid() const { return index != UINT32_MAX; }
    };

    class Resource
    {
    public:
        virtual ~Resource() = default;
        virtual const ResourceType GetResourceType() const = 0;
    };
}