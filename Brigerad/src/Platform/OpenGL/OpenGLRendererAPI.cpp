/**
 * @file   OpenGLRendererAPI.cpp
 * @author Samuel Martel
 * @date   2020/03/31
 *
 * @brief  Source for the OpenGLRendererAPI module.
 */
#include "OpenGLRendererAPI.h"

#include "../../Brigerad/Debug/Instrumentor.h"

#include <glad/glad.h>

namespace Brigerad
{
void OpenGLRendererAPI::Init()
{
    BR_PROFILE_FUNCTION();

    // Enable alpha blending.
    glEnable(GL_BLEND);
    // Set the way the blending is done.
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable depth testing, which tells OpenGL to check if the pixel to be
    // drawn is in front or behind the others.
    glEnable(GL_DEPTH_TEST);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // Disable byte-alignment restriction.
}

void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
{
    glClearColor(color.r, color.g, color.b, color.a);
}

void OpenGLRendererAPI::Clear()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount)
{
    uint32_t count = indexCount == 0 ? vertexArray->GetIndexBuffers()->GetCount() : indexCount;
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    glViewport(x, y, width, height);
}
}  // namespace Brigerad
