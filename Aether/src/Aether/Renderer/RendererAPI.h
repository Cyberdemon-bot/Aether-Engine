#pragma once

#include "Aether/Renderer/VertexArray.h"
#include <glm/glm.hpp>

namespace Aether {

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
        virtual void SetDepthFuncEqual(bool state) = 0;

        virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;
        virtual void DrawInstanced(const Ref<VertexArray>& vertexArray, uint32_t instanceCount) = 0;
		virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) = 0;
        virtual void DrawIndexedBaseVertex(const Ref<VertexArray>& vertexArray, uint32_t indexCount, void* indices, int32_t baseVertex) = 0;
		
		virtual void SetLineWidth(float width) = 0;
        
        static API GetAPI() { return s_API; }
        static Scope<RendererAPI> Create();
    private:
        static API s_API;
    };
}