#pragma once

#include "Brigerad/Core/Core.h"
#include "Brigerad/Core/KeyCodes.h"
#include "Brigerad/Core/MouseButtonCodes.h"

namespace Brigerad
{
class BRIGERAD_API Input
{
public:
    // Public API, static interface.
    static bool isKeyPressed(KeyCode keycode);

    static bool                    IsMouseButtonPressed(MouseCode button);
    static float                   GetMouseX();
    static float                   GetMouseY();
    static std::pair<float, float> GetMousePos();
};
}    // namespace Brigerad
