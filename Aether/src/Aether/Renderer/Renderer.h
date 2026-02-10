#pragma once

#include "Aether/Renderer/RenderCommand.h"
#include "Aether/Renderer/EditorCamera.h"
#include "Aether/Renderer/UniformBuffer.h"
#include "Aether/Resources/Shader.h"
#include "Aether/Resources/Mesh.h"

namespace Aether {

	class Renderer
	{
	public:
		static void Init();
		static void Shutdown();
		
		static void OnWindowResize(uint32_t width, uint32_t height);

		static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }
	private:
		struct SceneData
		{
			
		};

		static Scope<SceneData> s_SceneData;
	};
}
