#pragma once

#include "Aether/Renderer/VertexArray.h"
#include <glm/glm.hpp>

namespace Aether {

    enum class State
    {
        None = 0, EQUAL, LEQUAL, FRONT_CULL, BACK_CULL, ALWAYS
    };
    class AETHER_API RendererAPI 
    {
    public:
        enum class API {
            None = 0, OpenGL = 1
        };

    public:
        virtual ~RendererAPI() = default;
		virtual void Init() = 0;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
        virtual void SetClearColor(const glm::vec4& color) = 0;
        virtual void Clear() = 0;
        virtual void ClearColor() = 0;
        virtual void ClearDepth() = 0;
        virtual void SetDepthFuncEqual(State state) = 0;
        virtual void SetCullingMode(State state) = 0;

        virtual void DrawIndexed(VertexArray* vertexArray, uint32_t indexCount = 0) = 0;
        virtual void DrawInstanced(VertexArray* vertexArray, uint32_t instanceCount) = 0;
		virtual void DrawLines(VertexArray* vertexArray, uint32_t vertexCount) = 0;
        virtual void DrawIndexedLines(VertexArray* vertexArray, uint32_t indexCount) = 0;
        virtual void DrawIndexedBaseVertex(VertexArray* vertexArray, uint32_t indexCount, void* indices, int32_t baseVertex) = 0;
        virtual void DrawInstancedBaseVertex(VertexArray* vertexArray, uint32_t indexCount, void* indices, int32_t baseVertex, uint32_t instanceCount) = 0;
		
		virtual void SetLineWidth(float width) = 0;
        
        static API GetAPI() { return s_API; }
        static Scope<RendererAPI> Create();
    private:
        static API s_API;
    };
}