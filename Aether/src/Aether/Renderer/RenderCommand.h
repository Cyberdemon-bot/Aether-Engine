#pragma once
#include "Aether/Renderer/RendererAPI.h"

namespace Aether {
    class RenderCommand {
    public:
        static void Init() {
            s_RendererAPI->Init();
        }
        
        static void SetClearColor(const glm::vec4& color) {
            s_RendererAPI->SetClearColor(color);
        }

        static void Clear() {
            s_RendererAPI->Clear();
        }

        static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0)
		{
			s_RendererAPI->DrawIndexed(vertexArray, indexCount);
		}

		static void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount)
		{
			s_RendererAPI->DrawLines(vertexArray, vertexCount);
		}

        static void DrawInstanced(const Ref<VertexArray>& vertexArray, uint32_t instanceCount)
        {
            s_RendererAPI->DrawInstanced(vertexArray, instanceCount);
        }

		static void SetLineWidth(float width)
		{
			s_RendererAPI->SetLineWidth(width);
		}

        static void SetDepthFunc(GLenum func)
        {
            s_RendererAPI->SetDepthFunc(func);
        }

        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
        {
            s_RendererAPI->SetViewport(x, y, width, height);
        }
    private:
        static Scope<RendererAPI> s_RendererAPI;
    };
}