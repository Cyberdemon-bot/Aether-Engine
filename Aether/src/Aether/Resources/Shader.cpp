#include "Aether/Resources/Shader.h"
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

	void ShaderLibrary::Init()
    {
        s_ErrorShader = Shader::Create("assets/shaders/Basic.shader"); 
    }

    void ShaderLibrary::Shutdown()
    {
        s_Shaders.clear();
        s_ErrorShader.reset();
    }

    Ref<Shader> ShaderLibrary::Load(const std::string& filepath, UUID id)
    {
        if (s_Shaders.find(id) != s_Shaders.end())
            return s_Shaders[id];

        auto shader = Shader::Create(filepath);
        
        if (!shader) 
        {
            AE_CORE_ERROR("Shader Library: Failed to load '{0}'", filepath);
            return s_ErrorShader;
        }

        s_Shaders[id] = shader;
        return shader;
    }

    Ref<Shader> ShaderLibrary::Get(UUID id)
    {
        if (s_Shaders.find(id) != s_Shaders.end())
            return s_Shaders[id];

        AE_CORE_WARN("Shader Library: Shader ID not found!");
        return s_ErrorShader;
    }

    bool ShaderLibrary::Exists(UUID id)
    {
        return s_Shaders.find(id) != s_Shaders.end();
    }

	std::unordered_map<UUID, Ref<Shader>> ShaderLibrary::s_Shaders;
    Ref<Shader> ShaderLibrary::s_ErrorShader;
}