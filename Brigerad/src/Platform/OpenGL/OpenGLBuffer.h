#pragma once
#include "Brigerad/Renderer/Buffer.h"

namespace Brigerad
{
class OpenGLVertexBuffer : public VertexBuffer
{
    public:
    OpenGLVertexBuffer(uint32_t size);
    OpenGLVertexBuffer(float* vertices, uint32_t size);
    virtual ~OpenGLVertexBuffer() override;

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual const BufferLayout& GetLayout() override { return m_layout; }
    virtual void SetLayout(const BufferLayout& layout) override
    {
        m_layout = layout;
    }

    virtual void SetData(const void* data, uint32_t size) override;

    virtual const uint32_t GetId() const override { return m_rendererID; }

    private:
    uint32_t m_rendererID;
    BufferLayout m_layout;
};


class OpenGLIndexBuffer : public IndexBuffer
{
    public:
    OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
    virtual ~OpenGLIndexBuffer() override;

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual uint32_t GetCount() const override { return m_count; }

    virtual const uint32_t GetId() const override { return m_rendererID; }

    private:
    uint32_t m_rendererID;
    uint32_t m_count;
};
}  // namespace Brigerad