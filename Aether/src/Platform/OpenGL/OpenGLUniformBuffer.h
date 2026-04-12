#pragma once

#include "Aether/Renderer/UniformBuffer.h"
#include "OpenGLBase.h"

namespace Aether {

	class OpenGLUniformBuffer : public UniformBuffer
	{
	public:
		OpenGLUniformBuffer(uint32_t size);
		virtual ~OpenGLUniformBuffer();

		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void Bind(uint32_t slot = 0) const override;
	private:
		uint32_t m_RendererID = 0;
	};
}
