#pragma once

#include <glad/glad.h>
#include "VertexArray.h"
#include "IndexBuffer.h"
#include "Shader.h"

#if defined(_MSC_VER)
    #define DEBUG_BREAK() __debugbreak()
#elif defined(__clang__)
    #define DEBUG_BREAK() __builtin_debugtrap()
#endif

#define ASSERT(x) if (!(x)) DEBUG_BREAK();
#define GLCall(x) GLClearError(); x; ASSERT(GLLogCall(#x, __FILE__, __LINE__));

namespace Aether::Legacy {
    
    void GLClearError();
    bool GLLogCall(const char* function, const char* file, int line);

    class AETHER_API Renderer
    {
    public:
        void Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader) const;

        void Clear();
    };
}