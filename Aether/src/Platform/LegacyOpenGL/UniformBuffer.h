#pragma once

#include "LegacyBase.h"

namespace Aether::Legacy {

	class AETHER_API UniformBuffer
	{
	public:
		UniformBuffer(uint32_t size, uint32_t binding);
		~UniformBuffer();

		void SetData(const void* data, uint32_t size, uint32_t offset = 0);
	private:
		uint32_t m_RendererID = 0;
	};
}
