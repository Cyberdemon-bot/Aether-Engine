#include "aepch.h"
#include "Aether/Renderer/Shader.h"
#include "Aether/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLShader.h"

namespace Aether {
    Scope<Shader> Shader::CreateImpl(const std::string& filepath)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLShader>(filepath);
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Scope<Shader> Shader::CreateImpl(const ShaderProgramSource& source)
	{
		switch (Renderer::GetAPI())
		{
			case RendererAPI::API::None:    AE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::API::OpenGL:  return CreateScope<OpenGLShader>(source);
		}

		AE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}