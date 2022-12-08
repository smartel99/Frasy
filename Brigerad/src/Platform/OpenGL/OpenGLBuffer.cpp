#include "OpenGLBuffer.h"

#include "../../Brigerad/Debug/Instrumentor.h"

#include <glad/glad.h>

namespace Brigerad
{
/************************************************************************/
/* VertexBuffer                                                         */
/************************************************************************/

OpenGLVertexBuffer::OpenGLVertexBuffer(uint32_t size)
{
    BR_PROFILE_FUNCTION();

    glCreateBuffers(1, &m_rendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
    glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
}

OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size)
{
    BR_PROFILE_FUNCTION();

    glCreateBuffers(1, &m_rendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

OpenGLVertexBuffer::~OpenGLVertexBuffer()
{
    BR_PROFILE_FUNCTION();

    glDeleteBuffers(1, &m_rendererID);
}


void OpenGLVertexBuffer::Bind() const
{
    BR_PROFILE_FUNCTION();

    glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
}

void OpenGLVertexBuffer::Unbind() const
{
    BR_PROFILE_FUNCTION();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLVertexBuffer::SetData(const void* data, uint32_t size)
{
    glBindBuffer(GL_ARRAY_BUFFER, m_rendererID);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}


/************************************************************************/
/* IndexBuffer                                                          */
/************************************************************************/

OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count) : m_count(count)
{
    BR_PROFILE_FUNCTION();

    glCreateBuffers(1, &m_rendererID);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
}

OpenGLIndexBuffer::~OpenGLIndexBuffer()
{
    BR_PROFILE_FUNCTION();

    glDeleteBuffers(1, &m_rendererID);
}


void OpenGLIndexBuffer::Bind() const
{
    BR_PROFILE_FUNCTION();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_rendererID);
}

void OpenGLIndexBuffer::Unbind() const
{
    BR_PROFILE_FUNCTION();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

}    // namespace Brigerad
