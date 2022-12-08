/**
 * @file   Buffer.cpp
 * @author Samuel Martel
 * @date   2020/03/08
 *
 * @brief  Source for the Buffer module.
 */

#include "Buffer.h"

#include "Renderer.h"
#include "../../Platform/OpenGL/OpenGLBuffer.h"

namespace Brigerad
{

Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            BR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLVertexBuffer>(size);

        default:
            BR_CORE_ASSERT(false, "Invalid RendererAPI!");
            return nullptr;
    }
}

Ref<VertexBuffer> VertexBuffer::Create(float* vertices, uint32_t size)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            BR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLVertexBuffer>(vertices, size);
        default:
            BR_CORE_ASSERT(false, "Invalid RendererAPI!");
            return nullptr;
    }
}

Ref<IndexBuffer> IndexBuffer::Create(uint32_t* vertices, uint32_t size)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            BR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_unique<OpenGLIndexBuffer>(vertices, size);
        default:
            BR_CORE_ASSERT(false, "Invalid RendererAPI!");
            return nullptr;
    }
}

}  // namespace Brigerad
