#pragma once

#include "Brigerad/Renderer/VertexArray.h"


namespace Brigerad
{
class OpenGLVertexArray : public VertexArray
{
public:
    OpenGLVertexArray();
    virtual ~OpenGLVertexArray();

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
    virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override;

    virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const override;
    virtual const Ref<IndexBuffer>& GetIndexBuffers() const override;
private:
    std::vector<Ref<VertexBuffer>> m_vertexBuffers;
    Ref<IndexBuffer> m_indexBuffer;

    uint32_t m_rendererId;
};
}