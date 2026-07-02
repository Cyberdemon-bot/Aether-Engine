#include "aepch.h"
#include "Aether/Renderer/VertexArray.h"
#include "Aether/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLVertexArray.h"
#include "Aether/Core/ServiceManager.h"

namespace Aether {
    Scope<VertexArray> VertexArray::CreateImpl()
	{
		switch (ServiceManager::GetService<Renderer>()->GetAPI())
		{
			case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLVertexArray>();
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}