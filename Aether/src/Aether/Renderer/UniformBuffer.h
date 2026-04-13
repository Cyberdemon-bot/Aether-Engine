#pragma once

#include "Aether/Renderer/ResourceManager.h"

namespace Aether {

	class AETHER_API UniformBuffer : public Resource
	{
	public:
		virtual ~UniformBuffer() = default;
		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual void Bind(uint32_t slot = 0) const = 0;

		template<typename... Args>
        static Ref<UniformBuffer> Create(Args&&... args)
        {
            Scope<UniformBuffer> scope = CreateImpl(std::forward<Args>(args)...);
            return Ref<UniformBuffer>(std::move(scope));
        }

	private:
		static Scope<UniformBuffer> CreateImpl(uint32_t size);

		friend class ResourceManager;
	};

}
