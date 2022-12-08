/**
 * @file   FrameBuffer.cpp
 * @author Samuel Martel
 * @date   2020/06/13
 *
 * @brief  Source for the FrameBuffer module.
 */
#include "FrameBuffer.h"
#include "Renderer.h"

#include "../../Platform/OpenGL/OpenGLFrameBuffer.h"

namespace Brigerad
{
Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            BR_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL: return CreateRef<OpenGLFramebuffer>(spec);

        default: BR_CORE_ASSERT(false, "Invalid RendererAPI!"); return nullptr;
    }
}
}    // namespace Brigerad
