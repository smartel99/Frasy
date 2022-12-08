/**
 * @file   GraphicsContext.cpp
 * @author Samuel Martel
 * @date   2020/05/10
 *
 * @brief  Source for the GraphicsContext module.
 */
#include "GraphicsContext.h"

#include "../../Platform/OpenGL/OpenGLContext.h"
#include "Renderer.h"

namespace Brigerad
{
Scope<GraphicsContext> GraphicsContext::Create(void* window)
{
    switch (Renderer::GetAPI())
    {
        case RendererAPI::API::None:
            BR_CORE_ASSERT(false, "Renderer::API::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return std::make_unique<OpenGLContext>(static_cast<GLFWwindow*>(window));
    }

    BR_CORE_ASSERT(false, "Unknown rendering API!");
    return nullptr;
}
}    // namespace Brigerad
