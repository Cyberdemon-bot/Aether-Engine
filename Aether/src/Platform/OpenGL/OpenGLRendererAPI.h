#pragma once

#include "Aether/Renderer/RendererAPI.h"

namespace Aether {

	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;
		virtual void SetDepthFuncEqual(State state) override;
		virtual void SetCullingMode(State state) override;

		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void Clear() override;
		virtual void ClearColor() override;
		virtual void ClearDepth() override;

		virtual void DrawIndexed(VertexArray* vertexArray, uint32_t indexCount = 0) override;
		virtual void DrawInstanced(VertexArray* vertexArray, uint32_t instanceCount) override;
		virtual void DrawLines(VertexArray* vertexArray, uint32_t vertexCount) override;
		virtual void DrawIndexedLines(VertexArray* vertexArray, uint32_t indexCount) override;
		virtual void DrawIndexedBaseVertex(VertexArray* vertexArray, uint32_t indexCount, void* indices, int32_t baseVertex) override;
		virtual void DrawInstancedBaseVertex(VertexArray* vertexArray, uint32_t indexCount, void* indices, int32_t baseVertex, uint32_t instanceCount) override;

		virtual void SetLineWidth(float width) override;
	};
}
