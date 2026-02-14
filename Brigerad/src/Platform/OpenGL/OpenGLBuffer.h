#pragma once
#include "Brigerad/Renderer/Buffer.h"

namespace Brigerad
{
class OpenGLVertexBuffer : public VertexBuffer
{
    public:
    OpenGLVertexBuffer(uint32_t size);
    OpenGLVertexBuffer(float* vertices, uint32_t size);
    ~OpenGLVertexBuffer() override;

    void Bind() const override;
    void Unbind() const override;

    const BufferLayout& GetLayout() override { return m_layout; }
    void SetLayout(const BufferLayout& layout) override
    {
        m_layout = layout;
    }

    void SetData(const void* data, uint32_t size) override;

    uint32_t GetId() const override { return m_rendererID; }

    private:
    uint32_t m_rendererID;
    BufferLayout m_layout;
};


class OpenGLIndexBuffer : public IndexBuffer
{
    public:
    OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
    ~OpenGLIndexBuffer() override;

    void Bind() const override;
    void Unbind() const override;

    uint32_t GetCount() const override { return m_count; }

    uint32_t GetId() const override { return m_rendererID; }

    private:
    uint32_t m_rendererID;
    uint32_t m_count;
};
}  // namespace Brigerad
