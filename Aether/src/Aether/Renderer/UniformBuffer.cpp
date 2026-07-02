#include "aepch.h"
#include "UniformBuffer.h"
#include "Aether/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLUniformBuffer.h"
#include "Aether/Core/ServiceManager.h"

namespace Aether {

	Scope<UniformBuffer> UniformBuffer::CreateImpl(uint32_t size)
	{
		switch (ServiceManager::GetService<Renderer>()->GetAPI())
		{
			case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLUniformBuffer>(size);
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
