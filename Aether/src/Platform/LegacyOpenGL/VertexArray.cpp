#include "VertexArray.h"
#include "VertexBufferLayout.h"

namespace Aether::Legacy { 
    VertexArray::VertexArray()
    {
        GLCall(glGenVertexArrays(1, &m_RendererID));
    }

    VertexArray::~VertexArray()
    {
        GLCall(glDeleteVertexArrays(1, &m_RendererID));
    }

    void VertexArray::Bind() const
    {
        GLCall(glBindVertexArray(m_RendererID));
    }

    void VertexArray::Unbind() const
    {
        GLCall(glBindVertexArray(0));
    }

    void VertexArray::AddBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout)
    {
        Bind();
        vb.Bind();
        const auto& elements = layout.GetElements();
        unsigned int offset = 0;
        for (unsigned int i = 0; i < elements.size(); i++)
        {
            const auto& element = elements[i];
            GLCall(glEnableVertexAttribArray(i));
            GLCall(glVertexAttribPointer(i, element.count, element.type, element.normalized, layout.GetStride(), (const void*)(uintptr_t)offset));
            offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
        }
        
    }

    void VertexArray::AddInstanceBuffer(const VertexBuffer& vb, const VertexBufferLayout& layout,uint32_t startLocation)
    {
        Bind();
        vb.Bind();
        const auto& elements = layout.GetElements();
        unsigned int offset = 0;
        
        for (unsigned int i = 0; i < elements.size(); i++)
        {
            const auto& element = elements[i];
            uint32_t location = startLocation + i;
            
            GLCall(glEnableVertexAttribArray(location));
            GLCall(glVertexAttribPointer(location, element.count, element.type, element.normalized, layout.GetStride(), (const void*)(uintptr_t)offset));
            GLCall(glVertexAttribDivisor(location, 1));
            
            offset += element.count * VertexBufferElement::GetSizeOfType(element.type);
        }
    }
};