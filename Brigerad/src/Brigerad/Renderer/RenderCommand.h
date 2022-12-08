#pragma once

#include "RendererAPI.h"

#include "../Debug/Instrumentor.h"

namespace Brigerad
{
class RenderCommand
{
    public:
    inline static void SetClearColor(const glm::vec4& color)
    {
        s_rendererAPI->SetClearColor(color);
    }

    inline static void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        s_rendererAPI->SetViewport(x, y, width, height);
    }

    inline static void Clear() { s_rendererAPI->Clear(); }

    inline static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t count = 0)
    {
        BR_PROFILE_FUNCTION();
        s_rendererAPI->DrawIndexed(vertexArray, count);
    }

    inline static void Init() { s_rendererAPI->Init(); }

    private:
    static RendererAPI* s_rendererAPI;
};
}  // namespace Brigerad
