#pragma once

#include "aepch.h"
#include <glad/glad.h>
#include <iostream>

#if defined(_MSC_VER)
    #define DEBUG_BREAK() __debugbreak()
#elif defined(__clang__)
    #define DEBUG_BREAK() __builtin_debugtrap()
#endif

#define ASSERT(x) if (!(x)) DEBUG_BREAK();
#define GLCall(x) GLClearError(); x; ASSERT(GLLogCall(#x, __FILE__, __LINE__));

namespace Aether {
    
    inline void GLClearError()
    {
        while (glGetError() != GL_NO_ERROR);
    }
    inline bool GLLogCall(const char* function, const char* file, int line)
    {
        while (GLenum error = glGetError())
        {
            std::cout << "[OpenGL Error] (" << error << "): " << function << " " << file << ":" << line << std::endl;
            return false;
        }
        return true;
    }
}