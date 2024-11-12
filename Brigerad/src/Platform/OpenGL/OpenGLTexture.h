#pragma once
#include "Brigerad/Renderer/Texture.h"
#include <glad/glad.h>

#include <string_view>
#include <string>

namespace Brigerad {
class OpenGLTexture2D : public Texture2D {
public:
     OpenGLTexture2D(uint32_t width, uint32_t height, uint8_t channels);
     OpenGLTexture2D(std::string_view path);
    ~OpenGLTexture2D() override;

    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }

    uint32_t getRenderId() const override { return m_rendererID; }


    uint32_t         GetFormat() const override { return m_dataFormat; }
    std::string_view GetFilePath() const override { return m_path; }

    void SetData(void* data, uint32_t size) override;
    void Bind(uint32_t slot = 0) const override;

    bool operator==(const Texture& other) const override
    {
        return m_rendererID == ((OpenGLTexture2D&)other).m_rendererID;
    }

private:
    std::string m_path       = "";
    uint32_t    m_width      = 0;
    uint32_t    m_height     = 0;
    uint32_t    m_rendererID = 0;
    GLenum      m_internalFormat, m_dataFormat;
};
}    // namespace Brigerad
