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

	Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::string& filepath)
	{
		if (Exists(name)) return m_Shaders[name];
		auto shader = Shader::Create(filepath);
        m_Shaders[name] = shader;
        return shader;
	}

	Ref<Shader> ShaderLibrary::Get(const std::string& name)
	{
		auto it = m_Shaders.find(name);
        if (it == m_Shaders.end()) return m_ErrorShader; 
        return it->second;
	}

	bool ShaderLibrary::Exists(const std::string& name) const
	{
		return m_Shaders.find(name) != m_Shaders.end();
	}
}