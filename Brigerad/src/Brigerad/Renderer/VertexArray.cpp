/**
 * @file   VertexArray.cpp
 * @author Samuel Martel
 * @date   2020/03/31
 *
 * @brief  Source for the VertexArray module.
 */
#include "VertexArray.h"
#include "Renderer.h"

#include "../../Platform/OpenGL/OpenGLVertexArray.h"

namespace Brigerad
{
Ref<VertexArray> VertexArray::Create()
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            BR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL: return std::make_shared<OpenGLVertexArray>();
        default: BR_CORE_ASSERT(false, "Invalid RendererAPI!"); return nullptr;
    }
}
}    // namespace Brigerad
