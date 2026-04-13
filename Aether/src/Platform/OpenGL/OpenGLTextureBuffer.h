#pragma once

#include "Aether/Renderer/StorageBuffer.h"
#include "Platform/OpenGL/OpenGLBase.h"

namespace Aether {

	class AETHER_API OpenGLTextureBuffer : public StorageBuffer
	{
	public:
        OpenGLTextureBuffer(uint32_t size);
		virtual ~OpenGLTextureBuffer();
		virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
		virtual void Bind(uint32_t slot = 0) const override;
        virtual void Resize(uint32_t size) override;
		virtual uint32_t GetSize() const override { return m_Size; }
        virtual uint32_t GetRendererID() const override { return m_RendererID; }
	private:
        uint32_t m_RendererID = 0;   
		uint32_t m_BufferID = 0;    
		uint32_t m_Size = 0;        
	};
}
