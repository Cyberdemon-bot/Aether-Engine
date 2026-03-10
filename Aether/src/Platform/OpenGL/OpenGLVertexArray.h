#pragma once

#include "Aether/Renderer/VertexArray.h"
#include "OpenGLBase.h"

namespace Aether {
    class OpenGLVertexArray : public VertexArray
    {
    public:
        OpenGLVertexArray();
        virtual ~OpenGLVertexArray();

        virtual void Bind() const override;
        virtual void Unbind() const override;
        virtual uint32_t GetRendererID() const override { return m_RendererID; }

        virtual void AddVertexBuffer(VertexBuffer* vertexBuffer, uint32_t startLocation) override;
        virtual void AddVertexBuffer(VertexBuffer* vertexBuffer) override;

        virtual void AddInstanceBuffer(VertexBuffer* vertexBuffer, uint32_t startLocation) override;
        virtual void AddInstanceBuffer(VertexBuffer* vertexBuffer) override;
        virtual void SetIndexBuffer(IndexBuffer* indexBuffer) override;
        virtual uint32_t GetIndexCount() override;

        static Ref<VertexArray> Create();

        virtual bool operator==(const VertexArray& other) const override
		{
			return m_RendererID == other.GetRendererID();
		}
    private:
      uint32_t m_RendererID;
      uint32_t m_VertexBufferIndex = 0;
      std::vector<VertexBuffer*> m_VertexBuffers;
      IndexBuffer* m_IndexBuffer;
    };
}