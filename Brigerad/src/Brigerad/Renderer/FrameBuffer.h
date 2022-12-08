#pragma once

#include "Brigerad/Core/Core.h"

namespace Brigerad
{
struct FramebufferSpecification
{
    uint32_t width = 0, height = 0;
    //     FramebufferFormat Format;
    uint32_t samples = 1;

    bool swapChainTarget = false;
};


class Framebuffer
{
public:
    virtual ~Framebuffer()                                           = default;
    virtual const FramebufferSpecification& GetSpecification() const = 0;

    virtual void Bind()   = 0;
    virtual void Unbind() = 0;

    virtual void Resize(uint32_t width, uint32_t height) = 0;

    virtual uint32_t GetColorAttachmentRenderID() const = 0;

    static Ref<Framebuffer> Create(const FramebufferSpecification& spec);
};
}    // namespace Brigerad
