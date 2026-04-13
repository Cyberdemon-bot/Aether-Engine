#pragma once

#include "Aether/Renderer/Buffer.h"
#include "Aether/Renderer/ResourceManager.h"
namespace Aether {
    class AETHER_API VertexArray : public Resource
    {
    public:
        virtual ~VertexArray() = default;

        virtual void Bind() const = 0;
        virtual void Unbind() const = 0;
        virtual uint32_t GetRendererID() const = 0;

        virtual void AddVertexBuffer(VertexBuffer* vertexBuffer) = 0;
        virtual void AddVertexBuffer(VertexBuffer* vertexBuffer, uint32_t startLocation) = 0;
        virtual void AddInstanceBuffer(VertexBuffer* vertexBuffer, uint32_t startLocation) = 0;
        virtual void AddInstanceBuffer(VertexBuffer* vertexBuffer) = 0;
        virtual void SetIndexBuffer(IndexBuffer* indexBuffer) = 0;
        virtual uint32_t GetIndexCount() = 0;

        template<typename... Args>
        static Ref<VertexArray> Create(Args&&... args)
        {
            Scope<VertexArray> scope = CreateImpl(std::forward<Args>(args)...);
            return Ref<VertexArray>(std::move(scope));
        }

        virtual bool operator==(const VertexArray& other) const = 0;

    private:
		static Scope<VertexArray> CreateImpl();

		friend class ResourceManager;
    };
}