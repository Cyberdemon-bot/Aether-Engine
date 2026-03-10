#include "aepch.h"
#include "Aether/Renderer/Buffer.h"
#include "Aether/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLBuffer.h"

namespace Aether {
    Scope<VertexBuffer> VertexBuffer::CreateImpl(uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLVertexBuffer>(size);
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Scope<VertexBuffer> VertexBuffer::CreateImpl(float* vertices, uint32_t size)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLVertexBuffer>(vertices, size);
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Scope<IndexBuffer> IndexBuffer::CreateImpl(uint32_t* indices, uint32_t count)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLIndexBuffer>(indices, count);
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}