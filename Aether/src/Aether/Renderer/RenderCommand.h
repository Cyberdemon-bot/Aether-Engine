#pragma once
#include "Aether/Renderer/RendererAPI.h"

namespace Aether {
    class AETHER_API RenderCommand {
    public:
        static void Init() {
            s_RendererAPI->Init();
        }
        
        static void SetClearColor(const glm::vec4& color) {
            s_RendererAPI->SetClearColor(color);
        }

        static void Clear() 
        {
            s_RendererAPI->Clear();
        }

        static void ClearColor()
        {
            s_RendererAPI->ClearColor();
        }

        static void ClearDepth()
        {
            s_RendererAPI->ClearDepth();
        }

        static void DrawIndexed(VertexArray* vertexArray, uint32_t indexCount = 0)
		{
			s_RendererAPI->DrawIndexed(vertexArray, indexCount);
		}

		static void DrawLines(VertexArray* vertexArray, uint32_t vertexCount)
		{
			s_RendererAPI->DrawLines(vertexArray, vertexCount);
		}

        static void DrawInstanced(VertexArray* vertexArray, uint32_t instanceCount)
        {
            s_RendererAPI->DrawInstanced(vertexArray, instanceCount);
        }

        static void DrawIndexedBaseVertex(VertexArray* vertexArray, uint32_t indexCount, void* indices, int32_t baseVertex)
        {
            s_RendererAPI->DrawIndexedBaseVertex(vertexArray, indexCount, indices, baseVertex);
        }

        static void DrawInstancedBaseVertex(VertexArray* vertexArray, uint32_t indexCount, void* indices, int32_t baseVertex, uint32_t instanceCount)
        {
            s_RendererAPI->DrawInstancedBaseVertex(vertexArray, indexCount, indices, baseVertex, instanceCount);
        }

        static void DrawIndexedLines(VertexArray* vertexArray, uint32_t indexCount)
        {
            s_RendererAPI->DrawIndexedLines(vertexArray, indexCount);
        }

		static void SetLineWidth(float width)
		{
			s_RendererAPI->SetLineWidth(width);
		}

        static void SetDepthFuncEqual(State state)
        {
            s_RendererAPI->SetDepthFuncEqual(state);
        }

        static void SetCullingMode(State state)
        {
            s_RendererAPI->SetCullingMode(state);
        }

        static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
        {
            s_RendererAPI->SetViewport(x, y, width, height);
        }
    private:
        static Scope<RendererAPI> s_RendererAPI;
    };
}