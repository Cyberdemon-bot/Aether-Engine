#pragma once

#include "glm/glm.hpp"

#include "Platform/LegacyOpenGL/VertexArray.h"
#include "Platform/LegacyOpenGL/VertexBuffer.h"
#include "Platform/LegacyOpenGL/IndexBuffer.h"
#include "Platform/LegacyOpenGL/Shader.h"
#include "Platform/LegacyOpenGL/Texture.h"
#include "Platform/LegacyOpenGL/VertexBufferLayout.h"
#include "Platform/LegacyOpenGL/FrameBuffer.h"
#include "Aether/Renderer/Legacy/Camera.h"

namespace Aether::Legacy {

    class AETHER_API LegacyAPI {
    public:
        static void Init();
        static void SetViewport(int x, int y, int width, int height);
        
        static void SetClearColor(const glm::vec4& color);
        static void Clear();

        static void Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader);
    };

}