#pragma once
#include "Brigerad/Renderer/Texture.h"
#include <glad/glad.h>

namespace Brigerad
{
class OpenGLTexture2D : public Texture2D
{
public:
    OpenGLTexture2D(uint32_t width, uint32_t height, uint8_t channels);
    OpenGLTexture2D(const std::string& path);
    virtual ~OpenGLTexture2D() override;

    virtual uint32_t GetWidth() const override { return m_width; }
    virtual uint32_t GetHeight() const override { return m_height; }

    virtual uint32_t GetRenderID() const override { return m_rendererID; }


    virtual uint32_t           GetFormat() const override { return m_dataFormat; }
    virtual const std::string& GetFilePath() const override { return m_path; }

    virtual void SetData(void* data, uint32_t size) override;
    virtual void Bind(uint32_t slot = 0) const override;

    virtual bool operator==(const Texture& other) const override
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
