#include "Platform/OpenGL/OpenGLVertexArray.h"

namespace Aether {
    static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:    return GL_FLOAT;
			case ShaderDataType::Float2:   return GL_FLOAT;
			case ShaderDataType::Float3:   return GL_FLOAT;
			case ShaderDataType::Float4:   return GL_FLOAT;
			case ShaderDataType::Mat3:     return GL_FLOAT;
			case ShaderDataType::Mat4:     return GL_FLOAT;
			case ShaderDataType::Int:      return GL_INT;
			case ShaderDataType::Int2:     return GL_INT;
			case ShaderDataType::Int3:     return GL_INT;
			case ShaderDataType::Int4:     return GL_INT;
			case ShaderDataType::Bool:     return GL_BOOL;
            case ShaderDataType::None:     break;
		}

		AE_CORE_ASSERT(false, "Unknown ShaderDataType!");
		return 0;
	}

    OpenGLVertexArray::OpenGLVertexArray()
    {
        GLCall(glGenVertexArrays(1, &m_RendererID));
    }

    OpenGLVertexArray::~OpenGLVertexArray()
    {
        GLCall(glDeleteBuffers(1, &m_RendererID));
    }

    void OpenGLVertexArray::Bind() const 
    {
        GLCall(glBindVertexArray(m_RendererID));
    }

    void OpenGLVertexArray::Unbind() const
    {
        GLCall(glBindVertexArray(0));
    }

    void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
    {
        AddVertexBuffer(vertexBuffer, m_VertexBufferIndex);
    }

    void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer, uint32_t startLocation)
    {
        AE_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout");
        AE_CORE_ASSERT(startLocation >= m_VertexBufferIndex, "Vertex buffer location {0} conflicts with existing location {1}", startLocation, m_VertexBufferIndex);

        GLCall(glBindVertexArray(m_RendererID));
        vertexBuffer->Bind();

        uint32_t index = startLocation;

        const auto& layout = vertexBuffer->GetLayout();
        for(const auto& element : layout)
        {
            switch(element.Type)
            {
                case ShaderDataType::Float:  
                case ShaderDataType::Float2: 
                case ShaderDataType::Float3: 
                case ShaderDataType::Float4: 
                {
                    GLCall(glEnableVertexAttribArray(index));
                    GLCall(glVertexAttribPointer(
                        index,
                        element.GetComponentCount(),
                        ShaderDataTypeToOpenGLBaseType(element.Type),
                        element.Normalized ? GL_TRUE : GL_FALSE,
                        layout.GetStride(),
                        (const void*)element.Offset
                    ));
                    index++;
                    break;
                }
                case ShaderDataType::Int:    
                case ShaderDataType::Int2:   
                case ShaderDataType::Int3:  
                case ShaderDataType::Int4:  
                case ShaderDataType::Bool:  
                {
                    GLCall(glEnableVertexAttribArray(index));
                    GLCall(glVertexAttribIPointer(
                        index,
                        element.GetComponentCount(),
                        ShaderDataTypeToOpenGLBaseType(element.Type),
                        layout.GetStride(),
                        (const void*)element.Offset
                    ));
                    index++;
                    break;
                }
                case ShaderDataType::Mat3:  
                case ShaderDataType::Mat4:
                {
                    uint32_t count = element.GetComponentCount();
                    for (uint32_t i = 0; i < count; i++)
                    {
                        GLCall(glEnableVertexAttribArray(index));
                        GLCall(glVertexAttribPointer(index,
                            count,
                            ShaderDataTypeToOpenGLBaseType(element.Type),
                            element.Normalized ? GL_TRUE : GL_FALSE,
                            layout.GetStride(),
                            (const void*)(element.Offset + sizeof(float) * count * i)
                        ));
                        glVertexAttribDivisor(index, 0);
                        index++;
                    }
                    break;
                } 
                case ShaderDataType::None: break;
            }
        }
        m_VertexBufferIndex = std::max(m_VertexBufferIndex, index);
        m_VertexBuffers.push_back(vertexBuffer);
    }

    void OpenGLVertexArray::AddInstanceBuffer(const Ref<VertexBuffer>& vertexBuffer)
    {
        AddInstanceBuffer(vertexBuffer, m_VertexBufferIndex);
    }

    void OpenGLVertexArray::AddInstanceBuffer(const Ref<VertexBuffer>& vertexBuffer, uint32_t startLocation)
    {
        AE_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout");
        AE_CORE_ASSERT(startLocation >= m_VertexBufferIndex, "Vertex buffer location {0} conflicts with existing location {1}", startLocation, m_VertexBufferIndex);

        GLCall(glBindVertexArray(m_RendererID));
        vertexBuffer->Bind();

        const auto& layout = vertexBuffer->GetLayout();
        uint32_t index = startLocation;

        for (const auto& element : layout)
        {
            switch (element.Type)
            {
                case ShaderDataType::Float:
                case ShaderDataType::Float2:
                case ShaderDataType::Float3:
                case ShaderDataType::Float4:
                {
                    GLCall(glEnableVertexAttribArray(index));
                    GLCall(glVertexAttribPointer(
                        index,
                        element.GetComponentCount(),
                        ShaderDataTypeToOpenGLBaseType(element.Type),
                        element.Normalized ? GL_TRUE : GL_FALSE,
                        layout.GetStride(),
                        (const void*)element.Offset
                    ));
                    GLCall(glVertexAttribDivisor(index, 1)); 
                    index++;
                    break;
                }
                case ShaderDataType::Int:
                case ShaderDataType::Int2:
                case ShaderDataType::Int3:
                case ShaderDataType::Int4:
                case ShaderDataType::Bool:
                {
                    GLCall(glEnableVertexAttribArray(index));
                    GLCall(glVertexAttribIPointer(
                        index,
                        element.GetComponentCount(),
                        ShaderDataTypeToOpenGLBaseType(element.Type),
                        layout.GetStride(),
                        (const void*)element.Offset
                    ));
                    GLCall(glVertexAttribDivisor(index, 1));
                    index++;
                    break;
                }
                case ShaderDataType::Mat3:
                case ShaderDataType::Mat4:
                {
                    uint32_t count = element.GetComponentCount();
                    for (uint32_t i = 0; i < count; i++)
                    {
                        GLCall(glEnableVertexAttribArray(index));
                        GLCall(glVertexAttribPointer(
                            index,
                            count,
                            ShaderDataTypeToOpenGLBaseType(element.Type),
                            element.Normalized ? GL_TRUE : GL_FALSE,
                            layout.GetStride(),
                            (const void*)(element.Offset + sizeof(float) * count * i)
                        ));
                        GLCall(glVertexAttribDivisor(index, 1));
                        index++;
                    }
                    break;
                }
                case ShaderDataType::None: break;
            }
        }
        m_VertexBufferIndex = std::max(m_VertexBufferIndex, index);
        m_VertexBuffers.push_back(vertexBuffer);
    }

    void OpenGLVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
    {
        GLCall(glBindVertexArray(m_RendererID));
        indexBuffer->Bind();

        m_IndexBuffer = indexBuffer;
    }
}