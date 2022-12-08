/**
 * @file   RenderCommand.cpp
 * @author Samuel Martel
 * @date   2020/03/31
 *
 * @brief  Source for the RenderCommand module.
 */
#include "RenderCommand.h"
#include "../../Platform/OpenGL/OpenGLRendererAPI.h"

namespace Brigerad
{
RendererAPI* RenderCommand::s_rendererAPI = new OpenGLRendererAPI;
}
