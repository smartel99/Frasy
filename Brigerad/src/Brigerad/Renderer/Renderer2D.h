#pragma once

#include "Brigerad/Renderer/OrthographicCamera.h"
#include "Brigerad/Renderer/Camera.h"
#include "Brigerad/Renderer/Texture.h"
#include "Brigerad/Renderer/SubTexture2D.h"

#include <string>


namespace Brigerad
{
class Renderer2D
{
public:
    static void Init();
    static void Shutdown();

    static void BeginScene(const OrthographicCamera& camera);
    static void BeginScene(const Camera& camera, const glm::mat4& transform);
    static void EndScene();
    static void Flush();

    static long long GetFrameCount();

    /* ------------------------------------------------------------------------- */
    /* Primitives -------------------------------------------------------------- */
    /* ------------------------------------------------------------------------- */

    static void DrawString(const glm::vec2& pos, const std::string& text, float scale = 1.0f);
    static void DrawString(const glm::vec3& pos, const std::string& text, float scale = 1.0f);

    // ----- DRAW QUAD -----
    static void DrawQuad(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color);
    static void DrawQuad(const glm::vec3& pos, const glm::vec2& size, const glm::vec4& color);
    static void DrawQuad(const glm::vec2&      pos,
                         const glm::vec2&      size,
                         const Ref<Texture2D>& texture,
                         const glm::vec2&      textScale = glm::vec2(1.0f),
                         const glm::vec4&      tint      = glm::vec4(1.0f));
    static void DrawQuad(const glm::vec3&      pos,
                         const glm::vec2&      size,
                         const Ref<Texture2D>& texture,
                         const glm::vec2&      textScale = glm::vec2(1.0f),
                         const glm::vec4&      tint      = glm::vec4(1.0f));
    static void DrawQuad(const glm::vec2&         pos,
                         const glm::vec2&         size,
                         const Ref<SubTexture2D>& texture,
                         const glm::vec2&         textScale = glm::vec2(1.0f),
                         const glm::vec4&         tint      = glm::vec4(1.0f));
    static void DrawQuad(const glm::vec3&         pos,
                         const glm::vec2&         size,
                         const Ref<SubTexture2D>& texture,
                         const glm::vec2&         textScale = glm::vec2(1.0f),
                         const glm::vec4&         tint      = glm::vec4(1.0f));

    static void DrawQuad(const glm::mat4& transform, const glm::vec4& color);
    static void DrawQuad(const glm::mat4&      transform,
                         const Ref<Texture2D>& texture,
                         const glm::vec2&      textScale = glm::vec2(1.0f),
                         const glm::vec4&      tint      = glm::vec4(1.0f));
    static void DrawQuad(const glm::mat4&         transform,
                         const Ref<SubTexture2D>& texture,
                         const glm::vec2&         textScale = glm::vec2(1.0f),
                         const glm::vec4&         tint      = glm::vec4(1.0f));

    // ----- DRAW ROTATED QUAD -----
    static void DrawRotatedQuad(const glm::vec2& pos,
                                const glm::vec2& size,
                                const glm::vec4& color,
                                float            rotation = 0);
    static void DrawRotatedQuad(const glm::vec3& pos,
                                const glm::vec2& size,
                                const glm::vec4& color,
                                float            rotation = 0);
    static void DrawRotatedQuad(const glm::vec2&      pos,
                                const glm::vec2&      size,
                                const Ref<Texture2D>& texture,
                                const glm::vec2&      textScale = glm::vec2(1.0f),
                                const glm::vec4&      tint      = glm::vec4(1.0f),
                                float                 rotation  = 0);
    static void DrawRotatedQuad(const glm::vec3&      pos,
                                const glm::vec2&      size,
                                const Ref<Texture2D>& texture,
                                const glm::vec2&      textScale = glm::vec2(1.0f),
                                const glm::vec4&      tint      = glm::vec4(1.0f),
                                float                 rotation  = 0);
    static void DrawRotatedQuad(const glm::vec2&         pos,
                                const glm::vec2&         size,
                                const Ref<SubTexture2D>& texture,
                                const glm::vec2&         textScale = glm::vec2(1.0f),
                                const glm::vec4&         tint      = glm::vec4(1.0f),
                                float                    rotation  = 0);
    static void DrawRotatedQuad(const glm::vec3&         pos,
                                const glm::vec2&         size,
                                const Ref<SubTexture2D>& texture,
                                const glm::vec2&         textScale = glm::vec2(1.0f),
                                const glm::vec4&         tint      = glm::vec4(1.0f),
                                float                    rotation  = 0);


    // Statistics
    struct Statistics
    {
        uint32_t drawCalls = 0;
        uint32_t quadCount = 0;

        uint32_t GetTotalVertexCount() { return quadCount * 4; }
        uint32_t GetTotalIndexCount() { return quadCount * 6; }
    };
    static Statistics GetStats();
    static void       ResetStats();

private:
    static void FlushAndReset();
};
}    // namespace Brigerad
