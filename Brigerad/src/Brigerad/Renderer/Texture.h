#pragma once

#include "../Core/Core.h"

#include <string>


namespace Brigerad
{
class Texture
{
public:
    virtual ~Texture()                                           = default;
    [[nodiscard]] virtual uint32_t           GetWidth() const    = 0;
    [[nodiscard]] virtual uint32_t           GetHeight() const   = 0;
    [[nodiscard]] virtual uint32_t           GetFormat() const   = 0;
    [[nodiscard]] virtual const std::string& GetFilePath() const = 0;

    [[nodiscard]] virtual uint32_t getRenderId() const = 0;

    virtual void SetData(void* data, uint32_t size) = 0;

    virtual void Bind(uint32_t slot = 0) const = 0;

    virtual bool operator==(const Texture& other) const = 0;
};

class Texture2D : public Texture
{
public:
    static Ref<Texture2D> Create(uint32_t width, uint32_t height, uint8_t channels = 4);
    static Ref<Texture2D> Create(const std::string& path);
};
}    // namespace Brigerad
