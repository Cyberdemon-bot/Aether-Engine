#pragma once

#include "Aether/Renderer/ResourceManager.h"

namespace Aether {

	class AETHER_API StorageBuffer : public Resource
	{
	public:
		virtual ~StorageBuffer() = default;
		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;
		virtual void Bind(uint32_t slot = 0) const = 0;
		virtual void Resize(uint32_t size) = 0;
		virtual uint32_t GetSize() const = 0;
		virtual uint32_t GetRendererID() const = 0;

		template<typename... Args>
        static Ref<StorageBuffer> Create(Args&&... args)
        {
            Scope<StorageBuffer> scope = CreateImpl(std::forward<Args>(args)...);
            return Ref<StorageBuffer>(std::move(scope));
        }

	private:
		static Scope<StorageBuffer> CreateImpl(uint32_t size);

		friend class ResourceManager;
	};
}
