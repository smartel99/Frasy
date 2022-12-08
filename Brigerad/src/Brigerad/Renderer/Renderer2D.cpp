#include "Renderer2D.h"

#include "FontAtlas.h"
#include "RenderCommand.h"
#include "Shader.h"
#include "VertexArray.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Brigerad
{
/**
 * @brief   Data structure that contains all the information needed by a quad.
 */
struct QuadVertex
{
    glm::vec3 position;        // Screen coordinates of the quad.
    glm::vec4 color;           // Color of the quad.
    glm::vec2 texCoord;        // Coordinates to sample the texture from.
    glm::vec2 tilingFactor;    // Scaling applied to the sampled texture.
    float     texIndex;        // Index of the texture to use.
    float     isText;          // Is the quad text data.
};

/**
 * @brief   Runtime data structure that contains all the things needed by the
 *          renderer at runtime.
 */
struct Renderer2DData
{
    // Maximum amount of quads a single draw call can handle.
    static const uint32_t maxQuads        = 100000;          // Max 10k quads.
    static const uint32_t maxVertices     = maxQuads * 4;    // 4 vertices per quad.
    static const uint32_t maxIndices      = maxQuads * 6;    // 6 indices per quad.
    static const uint32_t maxTextureSlots = 32;    // Max number of textures per draw call.

    Ref<VertexArray>  vertexArray;
    Ref<VertexBuffer> vertexBuffer;
    Ref<Shader>       textureShader;    // Shader used at runtime.
    Ref<Texture2D>    whiteTexture;     // Default empty texture for flat color quads.
    Ref<FontAtlas>    font;             // Texture used for text.

    long long frameCount = 0;    // Frames rendered since the start of the application.

    // Number of quads queued to be drawn in this frame.
    uint32_t quadIndexCount = 0;
    // Origin of the buffer of queue of quads.
    QuadVertex* quadVertexBufferBase = nullptr;
    // Pointer to the location of the last quad currently queued.
    QuadVertex* quadVertexBufferPtr = nullptr;

    // CPU-sided representation of the texture memory in the GPU.
    std::array<Ref<Texture2D>, maxTextureSlots> textureSlots;
    // Current index of the last texture in the texture buffer.
    uint32_t textureSlotIndex = 2;    // 0 = white texture.
                                      // 1 = Font map.

    glm::vec4 quadVertexPosition[4] = {glm::vec4 {0.0f}};

    Renderer2D::Statistics stats;
};

// Instance of the runtime data for the renderer.
static Renderer2DData s_data;

/**
 * @brief   Initialize the 2D renderer.
 *          This sets everything up to be able to render, amongst other things, quads.
 */
void Renderer2D::Init()
{
    BR_PROFILE_FUNCTION();

    // Instantiate the vertex array used by the 2D renderer.
    s_data.vertexArray = VertexArray::Create();

    // Instantiate the vertex buffer to contain the maximum possible number
    // of quads that can be rendered in a single draw call.
    s_data.vertexBuffer = VertexBuffer::Create(s_data.maxVertices * sizeof(QuadVertex));

    // Set up the layout of the shader.
    s_data.vertexBuffer->SetLayout({{ShaderDataType::Float3, "a_position"},
                                    {ShaderDataType::Float4, "a_color"},
                                    {ShaderDataType::Float2, "a_TexCoord"},
                                    {ShaderDataType::Float2, "a_TilingFactor"},
                                    {ShaderDataType::Float, "a_TexIndex"},
                                    {ShaderDataType::Float, "a_IsText"}});

    s_data.vertexArray->AddVertexBuffer(s_data.vertexBuffer);

    // Heap allocate a buffer for all quads that can be rendered in a single draw call.
    s_data.quadVertexBufferBase = new QuadVertex[s_data.maxVertices];

    // Heap allocate a buffer for all quad indices that can be rendered in a single draw call.
    uint32_t* quadIndices = new uint32_t[s_data.maxIndices];

    // Initialize the indices of all quads possible.
    // Ex:  Quad 1 -> { 0, 1, 2, 2, 3, 0 }
    //      Quad 2 -> { 4, 5, 6, 6, 7, 4 }
    //      ...
    //      Quad n -> { n, n+1, n+2, n+2, n+3, n }
    uint32_t offset = 0;
    for (uint32_t i = 0; i < s_data.maxIndices; i += 6)
    {
        quadIndices[i + 0] = offset + 0;
        quadIndices[i + 1] = offset + 1;
        quadIndices[i + 2] = offset + 2;

        quadIndices[i + 3] = offset + 2;
        quadIndices[i + 4] = offset + 3;
        quadIndices[i + 5] = offset + 0;

        offset += 4;
    }

    // Create an index buffer for all of the indices we just set.
    Ref<IndexBuffer> quadIB = IndexBuffer::Create(quadIndices, s_data.maxIndices);
    // Bind that index buffer to our vertex array.
    s_data.vertexArray->SetIndexBuffer(quadIB);
    // Free up the memory.
    delete[] quadIndices;

    // Create a 1x1 white texture that we will use with flat colored quads.
    s_data.whiteTexture       = Texture2D::Create(1, 1);
    uint32_t whiteTextureData = 0xFFFFFFFF;
    s_data.whiteTexture->SetData(&whiteTextureData, sizeof(whiteTextureData));

    // Load, compile and link the shader for 2D quads.
    // #TODO Find better place for those
    std::string vertexShader = R"(
#version 330 core
layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec4 a_Color;
layout(location = 2) in vec2 a_TexCoord;
layout(location = 3) in vec2 a_TilingFactor;
layout(location = 4) in float a_TexIndex;
layout(location = 5) in float a_IsText;

uniform mat4 u_ViewProjection;

out vec4 v_Color;
out vec2 v_TexCoord;
out vec2 v_TilingFactor;
out float v_TexIndex;
out float v_IsText;

void main()
{
    v_Color = a_Color;
    v_TexCoord = a_TexCoord;
    v_TilingFactor = a_TilingFactor;
    v_TexIndex = a_TexIndex;
    v_IsText = a_IsText;
    // Set the position depending on the model and the camera.
    gl_Position = u_ViewProjection* vec4(a_Position, 1.0);
})";

    std::string fragmentShader = R"(
#version 330 core

layout(location = 0) out vec4 color;

in vec4 v_Color;
in vec2 v_TexCoord;
in vec2 v_TilingFactor;
in float v_TexIndex;
in float v_IsText;

uniform sampler2D u_Textures[32];

void RenderTexture()
{
    vec4 texColor = v_Color;
    switch(int(v_TexIndex))
    {
        case 0: texColor *= texture(u_Textures[0], v_TexCoord * v_TilingFactor); break;
        case 1: texColor *= texture(u_Textures[1], v_TexCoord * v_TilingFactor); break;
        case 2: texColor *= texture(u_Textures[2], v_TexCoord * v_TilingFactor); break;
        case 3: texColor *= texture(u_Textures[3], v_TexCoord * v_TilingFactor); break;
        case 4: texColor *= texture(u_Textures[4], v_TexCoord * v_TilingFactor); break;
        case 5: texColor *= texture(u_Textures[5], v_TexCoord * v_TilingFactor); break;
        case 6: texColor *= texture(u_Textures[6], v_TexCoord * v_TilingFactor); break;
        case 7: texColor *= texture(u_Textures[7], v_TexCoord * v_TilingFactor); break;
        case 8: texColor *= texture(u_Textures[8], v_TexCoord * v_TilingFactor); break;
        case 9: texColor *= texture(u_Textures[9], v_TexCoord * v_TilingFactor); break;
        case 10: texColor *= texture(u_Textures[10], v_TexCoord * v_TilingFactor); break;
        case 11: texColor *= texture(u_Textures[11], v_TexCoord * v_TilingFactor); break;
        case 12: texColor *= texture(u_Textures[12], v_TexCoord * v_TilingFactor); break;
        case 13: texColor *= texture(u_Textures[13], v_TexCoord * v_TilingFactor); break;
        case 14: texColor *= texture(u_Textures[14], v_TexCoord * v_TilingFactor); break;
        case 15: texColor *= texture(u_Textures[15], v_TexCoord * v_TilingFactor); break;
        case 16: texColor *= texture(u_Textures[16], v_TexCoord * v_TilingFactor); break;
        case 17: texColor *= texture(u_Textures[17], v_TexCoord * v_TilingFactor); break;
        case 18: texColor *= texture(u_Textures[18], v_TexCoord * v_TilingFactor); break;
        case 19: texColor *= texture(u_Textures[19], v_TexCoord * v_TilingFactor); break;
        case 20: texColor *= texture(u_Textures[20], v_TexCoord * v_TilingFactor); break;
        case 21: texColor *= texture(u_Textures[21], v_TexCoord * v_TilingFactor); break;
        case 22: texColor *= texture(u_Textures[22], v_TexCoord * v_TilingFactor); break;
        case 23: texColor *= texture(u_Textures[23], v_TexCoord * v_TilingFactor); break;
        case 24: texColor *= texture(u_Textures[24], v_TexCoord * v_TilingFactor); break;
        case 25: texColor *= texture(u_Textures[25], v_TexCoord * v_TilingFactor); break;
        case 26: texColor *= texture(u_Textures[26], v_TexCoord * v_TilingFactor); break;
        case 27: texColor *= texture(u_Textures[27], v_TexCoord * v_TilingFactor); break;
        case 28: texColor *= texture(u_Textures[28], v_TexCoord * v_TilingFactor); break;
        case 29: texColor *= texture(u_Textures[29], v_TexCoord * v_TilingFactor); break;
        case 30: texColor *= texture(u_Textures[30], v_TexCoord * v_TilingFactor); break;
        case 31: texColor *= texture(u_Textures[31], v_TexCoord * v_TilingFactor); break;
    }
    color = texColor;
}

void RenderText()
{
    vec4 texColor = v_Color;

    switch(int(v_TexIndex))
    {
        case 0: texColor *=  vec4(1.0, 1.0, 1.0, texture(u_Textures[0], v_TexCoord ).r); break;
        case 1: texColor *=  vec4(1.0, 1.0, 1.0, texture(u_Textures[1], v_TexCoord ).r); break;
        case 2: texColor *=  vec4(1.0, 1.0, 1.0, texture(u_Textures[2], v_TexCoord ).r); break;
        case 3: texColor *=  vec4(1.0, 1.0, 1.0, texture(u_Textures[3], v_TexCoord ).r); break;
        case 4: texColor *=  vec4(1.0, 1.0, 1.0, texture(u_Textures[4], v_TexCoord ).r); break;
        case 5: texColor *=  vec4(1.0, 1.0, 1.0, texture(u_Textures[5], v_TexCoord ).r); break;
        case 6: texColor *=  vec4(1.0, 1.0, 1.0, texture(u_Textures[6], v_TexCoord ).r); break;
        case 7: texColor *=  vec4(1.0, 1.0, 1.0, texture(u_Textures[7], v_TexCoord ).r); break;
        case 8: texColor *=  vec4(1.0, 1.0, 1.0, texture(u_Textures[8], v_TexCoord ).r); break;
        case 9: texColor *=  vec4(1.0, 1.0, 1.0, texture(u_Textures[9], v_TexCoord ).r); break;
        case 10: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[10], v_TexCoord).r); break;
        case 11: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[11], v_TexCoord).r); break;
        case 12: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[12], v_TexCoord).r); break;
        case 13: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[13], v_TexCoord).r); break;
        case 14: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[14], v_TexCoord).r); break;
        case 15: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[15], v_TexCoord).r); break;
        case 16: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[16], v_TexCoord).r); break;
        case 17: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[17], v_TexCoord).r); break;
        case 18: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[18], v_TexCoord).r); break;
        case 19: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[19], v_TexCoord).r); break;
        case 20: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[20], v_TexCoord).r); break;
        case 21: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[21], v_TexCoord).r); break;
        case 22: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[22], v_TexCoord).r); break;
        case 23: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[23], v_TexCoord).r); break;
        case 24: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[24], v_TexCoord).r); break;
        case 25: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[25], v_TexCoord).r); break;
        case 26: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[26], v_TexCoord).r); break;
        case 27: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[27], v_TexCoord).r); break;
        case 28: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[28], v_TexCoord).r); break;
        case 29: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[29], v_TexCoord).r); break;
        case 30: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[30], v_TexCoord).r); break;
        case 31: texColor *= vec4(1.0, 1.0, 1.0, texture(u_Textures[31], v_TexCoord).r); break;

    }
    color = texColor;
    //color = vec4(1.0, 0.0,0.0,1.0);
}

void main()
{
    // TODO: This apparently doesn't work on AMD GPUs, need to be tested.
    //color = texture(u_Textures[int(v_TexIndex)], v_TexCoord * v_TilingFactor) * v_Color;
    if(int(v_IsText) == 1)
    {
        RenderText();
    }
    else
    {
        RenderTexture();
    }
})";
    s_data.textureShader       = Shader::Create("default2D", vertexShader, fragmentShader);
    s_data.textureShader->Bind();

    // Setup the texture uniform in the fragment shader.
    // This array contains all of the possible texture slots that the shader
    // can use.
    int32_t samplers[s_data.maxTextureSlots] = {0};
    for (int32_t i = 0; i < s_data.maxTextureSlots; i++)
    {
        samplers[i] = i;
    }
    s_data.textureShader->SetIntArray("u_Textures", samplers, s_data.maxTextureSlots);

    // Set the first texture slot to be the 1x1 white texture.
    s_data.textureSlots[0] = s_data.whiteTexture;

    s_data.quadVertexPosition[0] = {-0.5f, -0.5f, 0.0f, 1.0f};
    s_data.quadVertexPosition[1] = {0.5f, -0.5f, 0.0f, 1.0f};
    s_data.quadVertexPosition[2] = {0.5f, 0.5f, 0.0f, 1.0f};
    s_data.quadVertexPosition[3] = {-0.5f, 0.5f, 0.0f, 1.0f};

    // Load default font.
    s_data.font            = FontAtlas::Create("c:/windows/fonts/times.ttf");
    s_data.textureSlots[1] = s_data.font->GetFontMap();
}

/**
 * @brief   Gracefully shutdown the 2D renderer.
 *          Currently, nothing in particular needs to be done in that method.
 */
void Renderer2D::Shutdown()
{
    BR_PROFILE_FUNCTION();
    delete[] s_data.quadVertexBufferBase;
}

/**
 * @brief Set everything up in the 2D renderer to begin accepting new draw
 *        calls for this frame.
 *
 * @param camera An orthographic representation of the scene that is viewable
 *               by the user.
 */
void Renderer2D::BeginScene(const OrthographicCamera& camera)
{
    BR_PROFILE_FUNCTION();

    // Upload the view-projection matrix of the camera into the vertex shader.
    s_data.textureShader->Bind();
    s_data.textureShader->SetMat4("u_ViewProjection", camera.GetViewProjectionMatrix());

    // Reset the quad buffer.
    s_data.quadIndexCount      = 0;
    s_data.quadVertexBufferPtr = s_data.quadVertexBufferBase;

    // Reset the texture buffer.
    // We set it to 2 instead of 0 because slot 0 is reserved to the 1x1 white texture
    // and slot 1 is reserved to the font map texture.
    s_data.textureSlotIndex = 2;

    // Increment the frame rendered count.
    s_data.frameCount++;
}

void Renderer2D::BeginScene(const Camera& camera, const glm::mat4& transform)
{
    BR_PROFILE_FUNCTION();

    glm::mat4 viewProj = camera.GetProjection() * glm::inverse(transform);

    // Upload the view-projection matrix of the camera into the vertex shader.
    s_data.textureShader->Bind();
    s_data.textureShader->SetMat4("u_ViewProjection", viewProj);

    // Reset the quad buffer.
    s_data.quadIndexCount      = 0;
    s_data.quadVertexBufferPtr = s_data.quadVertexBufferBase;

    // Reset the texture buffer.
    // We set it to 2 instead of 0 because slot 0 is reserved to the 1x1 white texture
    // and slot 1 is reserved to the font map texture.
    s_data.textureSlotIndex = 2;

    // Increment the frame rendered count.
    s_data.frameCount++;
}


/**
 * @brief   End a scene.
 *          Calling this method makes the renderer draw the entire quad queue
 *          that was filled since the last call to Renderer2D::BeginScene.
 */
void Renderer2D::EndScene()
{
    BR_PROFILE_FUNCTION();

    // Get how many quads are in the queue.
    uint32_t dataSize =
      (uint32_t)((uint8_t*)s_data.quadVertexBufferPtr - (uint8_t*)s_data.quadVertexBufferBase);
    // Upload the queued quads data into the vertex buffer.
    s_data.vertexBuffer->SetData(s_data.quadVertexBufferBase, dataSize);

    // Draw all queued quads in a single call.
    Flush();
}


/**
 * @brief Bind all queued textures and render the queue.
 */
void Renderer2D::Flush()
{
    BR_PROFILE_FUNCTION();

    s_data.stats.drawCalls++;

    // Bind all active textures.
    for (uint32_t i = 0; i < s_data.textureSlotIndex; i++)
    {
        s_data.textureSlots[i]->Bind(i);
    }
    // Draw the entire vertex array.
    RenderCommand::DrawIndexed(s_data.vertexArray, s_data.quadIndexCount);
}

void Renderer2D::FlushAndReset()
{
    EndScene();

    // Reset the quad buffer.
    s_data.quadIndexCount      = 0;
    s_data.quadVertexBufferPtr = s_data.quadVertexBufferBase;

    // Reset the texture buffer.
    // We set it to 2 instead of 0 because slot 0 is reserved to the 1x1 white texture
    // and slot 1 is reserved to the font map texture.
    s_data.textureSlotIndex = 2;
}


/**
 * @brief   Get the number of frames that has been rendered since the beginning
 *          of the application.
 *
 * @return  long long The number of frames rendered since the beginning of the
 *          application.
 */
long long Renderer2D::GetFrameCount()
{
    return s_data.frameCount;
}

/* ------------------------------------------------------------------------- */
/* Primitives -------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */

// ----- DRAW QUAD -----

void Renderer2D::DrawString(const glm::vec2& pos, const std::string& text, float scale)
{
    DrawString({pos.x, pos.y, 0.0f}, text, scale);
}

void Renderer2D::DrawString(const glm::vec3& pos, const std::string& text, float scale)
{
    glm::vec3 currentPos = pos;
    BR_CORE_TRACE("{}, {}", pos.x, pos.y);
    for (const auto& c : text)
    {
        const auto& glyph    = s_data.font->GetCharacterTexture(c);
        glm::vec3   glyphPos = {currentPos.x + (glyph.m_offset.x * scale),
                                currentPos.y - (glyph.m_offset.y * scale),
                                currentPos.z};
        DrawQuad(glyphPos, {glyph.m_size.x * scale, glyph.m_size.y * scale}, glyph.m_texture);
        currentPos.x += glyph.m_advance * scale;
        // BR_TRACE("{}: pos: {}, {}, offset: {}, {}, size: {}, {}",
        //         c,
        //         glyphPos.x,
        //         glyphPos.y,
        //         (glyph.m_offset.x * scale),
        //         (glyph.m_offset.y * scale),
        //         glyph.m_size.x * scale,
        //         glyph.m_size.y * scale);
    }
}

/**
 * @brief Queue a flat-colored quad in a 2D space.
 *
 * @param pos The world coordinates where to render the quad, (X, Y)
 * @param size The size of the quad, (X, Y)
 * @param color The color of the quad, (R, G, B, A)
 */
void Renderer2D::DrawQuad(const glm::vec2& pos, const glm::vec2& size, const glm::vec4& color)
{
    DrawQuad(glm::vec3(pos.x, pos.y, 0.0f), size, color);
}

/**
 * @brief Queue a flat-colored quad in a 3D space.
 *
 * @param pos The world coordinates where to render the quad, (X, Y, Z)
 * @param size The size of the quad, (X, Y)
 * @param color The color of the quad, (R, G, B, A)
 */
void Renderer2D::DrawQuad(const glm::vec3& pos, const glm::vec2& size, const glm::vec4& color)
{
    BR_PROFILE_FUNCTION();
    glm::mat4 transform =
      glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    DrawQuad(transform, color);
}

/**
 * @brief Queue a textured quad in a 2D space.
 *
 * @param pos The world coordinates where to render the quad, (X, Y)
 * @param size The size of the quad, (X, Y)
 * @param texture The texture to apply on that quad.
 * @param textScale The scaling factor of the texture, (X, Y)
 * @param tint A tint to apply to the texture, (R, G, B, A)
 */
void Renderer2D::DrawQuad(const glm::vec2&      pos,
                          const glm::vec2&      size,
                          const Ref<Texture2D>& texture,
                          const glm::vec2&      textScale,
                          const glm::vec4&      tint)
{
    DrawQuad({pos.x, pos.y, 0.0f}, size, texture, textScale, tint);
}

/**
 * @brief Queue a textured quad in a 3D space.
 *
 * @param pos The world coordinates where to render the quad, (X, Y, Z)
 * @param size The size of the quad, (X, Y)
 * @param texture The texture to apply on that quad.
 * @param textScale The scaling factor of the texture, (X, Y)
 * @param tint A tint to apply to the texture, (R, G, B, A)
 */
void Renderer2D::DrawQuad(const glm::vec3&      pos,
                          const glm::vec2&      size,
                          const Ref<Texture2D>& texture,
                          const glm::vec2&      textScale,
                          const glm::vec4&      tint)
{
    BR_PROFILE_FUNCTION();
    glm::mat4 transform =
      glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    DrawQuad(transform, texture, textScale, tint);
}

void Renderer2D::DrawQuad(const glm::vec2&         pos,
                          const glm::vec2&         size,
                          const Ref<SubTexture2D>& texture,
                          const glm::vec2&         textScale,
                          const glm::vec4&         tint)
{
    DrawQuad({pos.x, pos.y, 0.0f}, size, texture, textScale, tint);
}

void Renderer2D::DrawQuad(const glm::vec3&         pos,
                          const glm::vec2&         size,
                          const Ref<SubTexture2D>& texture,
                          const glm::vec2&         textScale,
                          const glm::vec4&         tint)
{
    glm::mat4 transform =
      glm::translate(glm::mat4(1.0f), pos) * glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    DrawQuad(transform, texture, textScale, tint);
}

void Renderer2D::DrawQuad(const glm::mat4& transform, const glm::vec4& color)
{
    BR_PROFILE_FUNCTION();

    constexpr size_t    quadVertexCount = 4;
    constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    // If the quad queue is full:
    if (s_data.quadIndexCount >= Renderer2DData::maxIndices)
    {    // Render the queue and start a new one.
        FlushAndReset();
    }

    const float texIndex = 0.0f;    // White Texture.

    for (int i = 0; i < quadVertexCount; i++)
    {
        // Setup the vertex of the quad.
        s_data.quadVertexBufferPtr->position     = transform * s_data.quadVertexPosition[i];
        s_data.quadVertexBufferPtr->color        = color;
        s_data.quadVertexBufferPtr->texCoord     = textureCoords[i];
        s_data.quadVertexBufferPtr->tilingFactor = {1.0f, 1.0f};
        s_data.quadVertexBufferPtr->texIndex     = texIndex;
        s_data.quadVertexBufferPtr->isText       = 0;
        s_data.quadVertexBufferPtr++;
    }

    // A quad has 6 indices, increment by that many for the next quad.
    s_data.quadIndexCount += 6;

    s_data.stats.quadCount++;
}

void Renderer2D::DrawQuad(const glm::mat4&      transform,
                          const Ref<Texture2D>& texture,
                          const glm::vec2&      textScale,
                          const glm::vec4&      tint)
{
    BR_PROFILE_FUNCTION();

    constexpr size_t    quadVertexCount = 4;
    constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    // If the quad queue is full:
    if (s_data.quadIndexCount >= Renderer2DData::maxIndices)
    {
        // Render the queue and start a new one.
        FlushAndReset();
    }

    float textureIndex = 0.0f;

    // Look up in the texture queue for the texture.
    for (uint32_t i = 1; i < s_data.textureSlotIndex; i++)
    {
        if (*s_data.textureSlots[i].get() == *texture.get())
        {
            textureIndex = (float)i;
            break;
        }
    }

    // If the texture is not already in the queue:
    if (textureIndex == 0.0f)
    {
        // Add in to the queue.
        textureIndex                                 = (float)s_data.textureSlotIndex;
        s_data.textureSlots[s_data.textureSlotIndex] = texture;
        s_data.textureSlotIndex++;
    }

    for (int i = 0; i < quadVertexCount; i++)
    {
        // Setup the vertex of the quad.
        s_data.quadVertexBufferPtr->position     = transform * s_data.quadVertexPosition[i];
        s_data.quadVertexBufferPtr->color        = tint;
        s_data.quadVertexBufferPtr->texCoord     = textureCoords[i];
        s_data.quadVertexBufferPtr->tilingFactor = textScale;
        s_data.quadVertexBufferPtr->texIndex     = textureIndex;
        s_data.quadVertexBufferPtr->isText       = 0;
        s_data.quadVertexBufferPtr++;
    }

    // A quad has 6 indices, increment the indices count by that many.
    s_data.quadIndexCount += 6;

    s_data.stats.quadCount++;
}

void Renderer2D::DrawQuad(const glm::mat4&         transform,
                          const Ref<SubTexture2D>& texture,
                          const glm::vec2&         textScale,
                          const glm::vec4&         tint)
{

    constexpr size_t quadVertexCount = 4;
    const glm::vec2* textureCoords   = texture->GetTexCoords();

    // If the quad queue is full:
    if (s_data.quadIndexCount >= Renderer2DData::maxIndices)
    {
        // Render the queue and start a new one.
        FlushAndReset();
    }

    float textureIndex = 0.0f;

    // Look up in the texture queue for the texture.
    for (uint32_t i = 1; i < s_data.textureSlotIndex; i++)
    {
        if (*s_data.textureSlots[i].get() == *texture->GetTexture().get())
        {
            textureIndex = (float)i;
            break;
        }
    }

    // If the texture is not already in the queue:
    if (textureIndex == 0.0f)
    {
        // Add in to the queue.
        textureIndex                                 = (float)s_data.textureSlotIndex;
        s_data.textureSlots[s_data.textureSlotIndex] = texture->GetTexture();
        s_data.textureSlotIndex++;
    }

    for (int i = 0; i < quadVertexCount; i++)
    {
        // Setup the vertex of the quad.
        s_data.quadVertexBufferPtr->position     = transform * s_data.quadVertexPosition[i];
        s_data.quadVertexBufferPtr->color        = tint;
        s_data.quadVertexBufferPtr->texCoord     = textureCoords[i];
        s_data.quadVertexBufferPtr->tilingFactor = textScale;
        s_data.quadVertexBufferPtr->texIndex     = textureIndex;
        s_data.quadVertexBufferPtr->isText       = 0;
        s_data.quadVertexBufferPtr++;
    }

    // A quad has 6 indices, increment the indices count by that many.
    s_data.quadIndexCount += 6;

    s_data.stats.quadCount++;
}

// ----- DRAW ROTATED QUAD -----

/**
 * @brief Queued a flat colored rotated quad in 2D space.
 *
 * @param pos The world coordinates where to render the quad, (X, Y)
 * @param size The size of the quad, (X, Y)
 * @param color The color of the quad, (R, G, B, A)
 * @param rotation The rotation to apply to the quad, in degrees
 */
void Renderer2D::DrawRotatedQuad(const glm::vec2& pos,
                                 const glm::vec2& size,
                                 const glm::vec4& color,
                                 float            rotation)
{
    DrawRotatedQuad(glm::vec3(pos.x, pos.y, 0.0f), size, color, rotation);
}

/**
 * @brief Queued a flat colored rotated quad in 3D space.
 *
 * @param pos The world coordinates where to render the quad, (X, Y, Z)
 * @param size The size of the quad, (X, Y)
 * @param color The color of the quad, (R, G, B, A)
 * @param rotation The rotation to apply to the quad, in degrees
 */
void Renderer2D::DrawRotatedQuad(const glm::vec3& pos,
                                 const glm::vec2& size,
                                 const glm::vec4& color,
                                 float            rotation)
{
    BR_PROFILE_FUNCTION();

    constexpr size_t    quadVertexCount = 4;
    constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    // If the quad queue is full:
    if (s_data.quadIndexCount >= Renderer2DData::maxIndices)
    {
        // Render the queue and start a new one.
        FlushAndReset();
    }

    const float texIndex = 0.0f;    // White Texture.

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) *
                          glm::rotate(glm::mat4(1.0f), rotation, {0, 0, 1}) *
                          glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});

    for (int i = 0; i < quadVertexCount; i++)
    {
        // Setup the vertex of the quad.
        s_data.quadVertexBufferPtr->position     = transform * s_data.quadVertexPosition[i];
        s_data.quadVertexBufferPtr->color        = color;
        s_data.quadVertexBufferPtr->texCoord     = textureCoords[i];
        s_data.quadVertexBufferPtr->tilingFactor = {1.0f, 1.0f};
        s_data.quadVertexBufferPtr->texIndex     = texIndex;
        s_data.quadVertexBufferPtr->isText       = 0;
        s_data.quadVertexBufferPtr++;
    }

    // A quad has 6 indices, increment the count by that many.
    s_data.quadIndexCount += 6;

    s_data.stats.quadCount++;
}

/**
 * @brief Queue a textured rotated quad in a 2D space.
 *
 * @param pos The world coordinates where to render the quad, (X, Y)
 * @param size The size of the quad, (X, Y)
 * @param texture The texture to apply on that quad.
 * @param textScale The scaling factor of the texture, (X, Y)
 * @param tint A tint to apply to the texture, (R, G, B, A)
 * @param rotation The rotation to apply to the quad, in degrees
 */
void Renderer2D::DrawRotatedQuad(const glm::vec2&      pos,
                                 const glm::vec2&      size,
                                 const Ref<Texture2D>& texture,
                                 const glm::vec2&      textScale,
                                 const glm::vec4&      tint,
                                 float                 rotation)
{
    DrawRotatedQuad({pos.x, pos.y, 0.0f}, size, texture, textScale, tint, rotation);
}

/**
 * @brief Queue a textured rotated quad in a 3D space.
 *
 * @param pos The world coordinates where to render the quad, (X, Y, Z)
 * @param size The size of the quad, (X, Y)
 * @param texture The texture to apply on that quad.
 * @param textScale The scaling factor of the texture, (X, Y)
 * @param tint A tint to apply to the texture, (R, G, B, A)
 * @param rotation The rotation to apply to the quad, in degrees
 */
void Renderer2D::DrawRotatedQuad(const glm::vec3&      pos,
                                 const glm::vec2&      size,
                                 const Ref<Texture2D>& texture,
                                 const glm::vec2&      textScale,
                                 const glm::vec4&      tint,
                                 float                 rotation)
{
    BR_PROFILE_FUNCTION();

    constexpr size_t    quadVertexCount = 4;
    constexpr glm::vec2 textureCoords[] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};

    // If the quad queue is full:
    if (s_data.quadIndexCount >= Renderer2DData::maxIndices)
    {
        // Render the queue and start a new one.
        FlushAndReset();
    }

    // Check if the texture is already in the texture queue.
    float textureIndex = 0.0f;
    for (uint32_t i = 1; i < s_data.textureSlotIndex; i++)
    {
        if (*s_data.textureSlots[i].get() == *texture.get())
        {
            textureIndex = (float)i;
            break;
        }
    }

    // If the texture isn't already in the texture queue:
    if (textureIndex == 0.0f)
    {
        // Add it to the queue.
        textureIndex                                 = (float)s_data.textureSlotIndex;
        s_data.textureSlots[s_data.textureSlotIndex] = texture;
        s_data.textureSlotIndex++;
    }

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) *
                          glm::rotate(glm::mat4(1.0f), rotation, {0, 0, 1}) *
                          glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});


    for (int i = 0; i < quadVertexCount; i++)
    {
        // Setup the vertex of the quad.
        s_data.quadVertexBufferPtr->position     = transform * s_data.quadVertexPosition[i];
        s_data.quadVertexBufferPtr->color        = tint;
        s_data.quadVertexBufferPtr->texCoord     = textureCoords[i];
        s_data.quadVertexBufferPtr->tilingFactor = textScale;
        s_data.quadVertexBufferPtr->texIndex     = textureIndex;
        s_data.quadVertexBufferPtr->isText       = 0;
        s_data.quadVertexBufferPtr++;
    }

    // A quad has 6 indices, increment the number of indices by that many.
    s_data.quadIndexCount += 6;

    s_data.stats.quadCount++;
}

void Renderer2D::DrawRotatedQuad(const glm::vec2&         pos,
                                 const glm::vec2&         size,
                                 const Ref<SubTexture2D>& texture,
                                 const glm::vec2&         textScale,
                                 const glm::vec4&         tint,
                                 float                    rotation)
{
    DrawRotatedQuad({pos.x, pos.y, 0.0f}, size, texture, textScale, tint, rotation);
}

void Renderer2D::DrawRotatedQuad(const glm::vec3&         pos,
                                 const glm::vec2&         size,
                                 const Ref<SubTexture2D>& texture,
                                 const glm::vec2&         textScale,
                                 const glm::vec4&         tint,
                                 float                    rotation)
{

    BR_PROFILE_FUNCTION();

    constexpr size_t quadVertexCount = 4;
    const glm::vec2* textureCoords   = texture->GetTexCoords();

    // If the quad queue is full:
    if (s_data.quadIndexCount >= Renderer2DData::maxIndices)
    {
        // Render the queue and start a new one.
        FlushAndReset();
    }

    // Check if the texture is already in the texture queue.
    float textureIndex = 0.0f;
    for (uint32_t i = 1; i < s_data.textureSlotIndex; i++)
    {
        if (*s_data.textureSlots[i].get() == *texture->GetTexture().get())
        {
            textureIndex = (float)i;
            break;
        }
    }

    // If the texture isn't already in the texture queue:
    if (textureIndex == 0.0f)
    {
        // Add it to the queue.
        textureIndex                                 = (float)s_data.textureSlotIndex;
        s_data.textureSlots[s_data.textureSlotIndex] = texture->GetTexture();
        s_data.textureSlotIndex++;
    }

    glm::mat4 transform = glm::translate(glm::mat4(1.0f), pos) *
                          glm::rotate(glm::mat4(1.0f), rotation, {0, 0, 1}) *
                          glm::scale(glm::mat4(1.0f), {size.x, size.y, 1.0f});


    for (int i = 0; i < quadVertexCount; i++)
    {
        // Setup the vertex of the quad.
        s_data.quadVertexBufferPtr->position     = transform * s_data.quadVertexPosition[i];
        s_data.quadVertexBufferPtr->color        = tint;
        s_data.quadVertexBufferPtr->texCoord     = textureCoords[i];
        s_data.quadVertexBufferPtr->tilingFactor = textScale;
        s_data.quadVertexBufferPtr->texIndex     = textureIndex;
        s_data.quadVertexBufferPtr->isText       = 0;
        s_data.quadVertexBufferPtr++;
    }

    // A quad has 6 indices, increment the number of indices by that many.
    s_data.quadIndexCount += 6;

    s_data.stats.quadCount++;
}


Renderer2D::Statistics Renderer2D::GetStats()
{
    return s_data.stats;
}

void Renderer2D::ResetStats()
{
    memset(&s_data.stats, 0, sizeof(Statistics));
}

}    // namespace Brigerad
