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

	void ShaderLibrary::Init()
    {
        GetShaders().reserve(128);
        AE_CORE_INFO("ShaderLibrary initialized");
    }

    void ShaderLibrary::Shutdown()
    {
        GetShaders().clear();
    }

    void ShaderLibrary::Add(Ref<Shader> obj, UUID id)
    {
        auto& shaders = GetShaders();
        if (shaders.find(id) != shaders.end())
        {
            AE_CORE_ERROR("Shader Library: ID already exists");
            return;
        }

        if (!obj)
        {
            AE_CORE_ERROR("Shader Library: Cannot add null obj");
            return;
        }
        shaders[id] = obj;
    }

    Ref<Shader> ShaderLibrary::Get(UUID id)
    {
        auto& shaders = GetShaders();
        if (shaders.find(id) != shaders.end())
            return shaders[id];

        AE_CORE_WARN("Shader Library: ID not found!");
        return nullptr;
    }

    bool ShaderLibrary::Exists(UUID id)
    {
        auto& shaders = GetShaders();
        return shaders.find(id) != shaders.end();
    }

	std::unordered_map<UUID, Ref<Shader>>& ShaderLibrary::GetShaders()
    {
        static std::unordered_map<UUID, Ref<Shader>> s_Shaders;
        return s_Shaders;
    }
}