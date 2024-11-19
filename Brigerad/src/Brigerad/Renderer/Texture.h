#pragma once

#include "../Core/Core.h"

#include <string_view>


namespace Brigerad {
class Texture {
public:
    virtual ~Texture() = default;

    [[nodiscard]] virtual uint32_t         GetWidth() const    = 0;
    [[nodiscard]] virtual uint32_t         GetHeight() const   = 0;
    [[nodiscard]] virtual uint32_t         GetFormat() const   = 0;
    [[nodiscard]] virtual std::string_view GetFilePath() const = 0;

    [[nodiscard]] virtual uint32_t getRenderId() const = 0;

    virtual void SetData(void* data, uint32_t size) = 0;

    virtual void Bind(uint32_t slot = 0) const = 0;

    virtual bool operator==(const Texture& other) const = 0;
};

class Texture2D : public Texture {
public:
    ~Texture2D() override = default;

    static Ref<Texture2D> Create(uint32_t width, uint32_t height, uint8_t channels = 4);
    static Ref<Texture2D> Create(std::string_view path);
};
}    // namespace Brigerad
