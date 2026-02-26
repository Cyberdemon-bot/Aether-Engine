#include "Aether/Renderer/Shader.h"
#include "Aether/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLShader.h"

namespace Aether {
    Ref<Shader> Shader::Create(const std::string& filepath)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateRef<OpenGLShader>(filepath);
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}