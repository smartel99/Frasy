/**
 * @file   OpenGLVertexArray.cpp
 * @author Samuel Martel
 * @date   2020/03/31
 *
 * @brief  Source for the OpenGLVertexArray module.
 */

#include "OpenGLVertexArray.h"

#include "../../Brigerad/Debug/Instrumentor.h"

#include <glad/glad.h>

namespace Brigerad
{

static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type)
{
    switch (type)
    {
        case ShaderDataType::Float: return GL_FLOAT;
        case ShaderDataType::Float2: return GL_FLOAT;
        case ShaderDataType::Float3: return GL_FLOAT;
        case ShaderDataType::Float4: return GL_FLOAT;
        case ShaderDataType::Mat3: return GL_FLOAT;
        case ShaderDataType::Mat4: return GL_FLOAT;
        case ShaderDataType::Int: return GL_INT;
        case ShaderDataType::Int2: return GL_INT;
        case ShaderDataType::Int3: return GL_INT;
        case ShaderDataType::Int4: return GL_INT;
        case ShaderDataType::Bool: return GL_BOOL;
        case ShaderDataType::None:
        default: BR_CORE_ASSERT(false, "Invalid ShaderDataType!"); return 0;
    }
}

OpenGLVertexArray::OpenGLVertexArray()
{
    BR_PROFILE_FUNCTION();

    glCreateVertexArrays(1, &m_rendererId);
}

OpenGLVertexArray::~OpenGLVertexArray()
{
    BR_PROFILE_FUNCTION();

    glDeleteVertexArrays(1, &m_rendererId);
}

void OpenGLVertexArray::Bind() const
{
    BR_PROFILE_FUNCTION();
    glBindVertexArray(m_rendererId);
}


void OpenGLVertexArray::Unbind() const
{
    BR_PROFILE_FUNCTION();
    glBindVertexArray(0);
}


void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
{
    BR_PROFILE_FUNCTION();

    BR_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex buffer has no layout");

    glBindVertexArray(m_rendererId);
    vertexBuffer->Bind();

    uint32_t    index  = 0;
    const auto& layout = vertexBuffer->GetLayout();
    for (const auto& element : layout)
    {
        glEnableVertexAttribArray(index);
        // "'type cast': conversion from 'const uint32_t' to 'const void*' of greater
        // size." This is desired behavior.
#pragma warning(disable : 4312)
        glVertexAttribPointer(index,
                              element.GetComponentCount(),
                              ShaderDataTypeToOpenGLBaseType(element.type),
                              element.normalized ? GL_TRUE : GL_FALSE,
                              layout.GetStride(),
                              (const void*)element.offset);
#pragma warning(default : 4312)
        index++;
    }

    m_vertexBuffers.emplace_back(vertexBuffer);
}


void OpenGLVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
{
    glBindVertexArray(m_rendererId);
    indexBuffer->Bind();

    m_indexBuffer = indexBuffer;
}

const std::vector<Ref<VertexBuffer>>& OpenGLVertexArray::GetVertexBuffers() const
{
    return m_vertexBuffers;
}

const Ref<IndexBuffer>& OpenGLVertexArray::GetIndexBuffers() const
{
    return m_indexBuffer;
}

}    // namespace Brigerad
