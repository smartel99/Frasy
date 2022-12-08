#pragma once

#include "../Core/Core.h"

namespace Brigerad
{

class GraphicsContext
{
public:
    virtual ~GraphicsContext() = default;
    virtual void Init()        = 0;
    virtual void SwapBuffers() = 0;


    static Scope<GraphicsContext> Create(void* window);

private:
};
}    // namespace Brigerad
