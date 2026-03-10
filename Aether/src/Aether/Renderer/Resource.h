#pragma once

#include "Aether/Core/Base.h"

namespace Aether {
    enum class ResourceType
    {
       Texture2D, Shader, UniformBuffer,
       VertexBuffer, IndexBuffer, VertexArray, FrameBuffer
    };

    struct ResourceHandle
    {
        int index = -1, generation = -1;

        bool IsValid() const { return index < 0; }
    };

    class Resource
    {
    public:
        virtual ~Resource() = default;
        virtual const ResourceType GetResourceType() const = 0;
    };
}