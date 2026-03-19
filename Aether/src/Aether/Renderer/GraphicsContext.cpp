#include "aepch.h"
#include "Aether/Renderer/GraphicsContext.h"
#include "Platform/OpenGL/OpenGLContext.h"
#include "Aether/Core/Assert.h"

namespace Aether {

	Scope<GraphicsContext> GraphicsContext::Create(void* window)
	{
		return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));

		AE_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}