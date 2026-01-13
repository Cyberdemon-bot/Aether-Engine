#include "aepch.h"
#include "Aether/Renderer/RenderCommand.h"

namespace Aether {

	Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

}