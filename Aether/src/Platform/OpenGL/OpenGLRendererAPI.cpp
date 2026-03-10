#include "aepch.h"
#include "OpenGLRendererAPI.h"
#include <glad/glad.h>

namespace Aether 
{

    void OpenGLRendererAPI::Init()
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_LINE_SMOOTH);
	}

	void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::SetDepthFuncEqual(State state)
    {
		if (state == State::None) glDisable(GL_DEPTH_TEST);
        if (state == State::EQUAL || state == State::None) glDepthFunc(GL_EQUAL);
		if (state == State::LEQUAL) glDepthFunc(GL_LEQUAL);
		if (state == State::ALWAYS) glDepthFunc(GL_ALWAYS);
    }

	void OpenGLRendererAPI::SetCullingMode(State state)
	{
		if (state == State::None)
		{
			glDisable(GL_CULL_FACE);
			return;
		}
		glEnable(GL_CULL_FACE);
		if (state == State::FRONT_CULL) glCullFace(GL_FRONT);
		if (state == State::BACK_CULL) glCullFace(GL_BACK);
	}

    void OpenGLRendererAPI::SetClearColor(const glm::vec4& color) {
        glClearColor(color.r, color.g, color.b, color.a);
    }

	void OpenGLRendererAPI::DrawIndexedBaseVertex(VertexArray* vertexArray, uint32_t indexCount, void* indices, int32_t baseVertex)
	{
		vertexArray->Bind();
		glDrawElementsBaseVertex(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, indices, baseVertex);
	}

	void OpenGLRendererAPI::DrawInstancedBaseVertex(VertexArray* vertexArray, uint32_t indexCount, void* indices, int32_t baseVertex, uint32_t instanceCount)
	{
		vertexArray->Bind();
    	glDrawElementsInstancedBaseVertex(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, indices, instanceCount, baseVertex);
	}

    void OpenGLRendererAPI::Clear() 
	{
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

	void OpenGLRendererAPI::ClearColor()
	{
		glClear(GL_COLOR_BUFFER_BIT);
	}

	
	void OpenGLRendererAPI::ClearDepth() 
	{
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::DrawIndexed(VertexArray* vertexArray, uint32_t indexCount)
	{
		vertexArray->Bind();
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexCount();
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::DrawLines(VertexArray* vertexArray, uint32_t vertexCount)
	{
		vertexArray->Bind();
		glDrawArrays(GL_LINES, 0, vertexCount);
	}

	void OpenGLRendererAPI::DrawIndexedLines(VertexArray* vertexArray, uint32_t indexCount)
	{
		vertexArray->Bind();
		uint32_t count = indexCount ? indexCount : vertexArray->GetIndexCount();
		glDrawElements(GL_LINES, count, GL_UNSIGNED_INT, nullptr);
	}

	void OpenGLRendererAPI::DrawInstanced(VertexArray* vertexArray, uint32_t instanceCount)
    {
        vertexArray->Bind();
		uint32_t count = vertexArray->GetIndexCount();
        glDrawElementsInstanced(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr, instanceCount);
    }

	void OpenGLRendererAPI::SetLineWidth(float width)
	{
		glLineWidth(width);
	}

}