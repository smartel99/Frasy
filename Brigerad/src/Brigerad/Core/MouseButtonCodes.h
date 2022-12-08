#pragma once

namespace Brigerad
{
typedef enum class MouseCode : uint16_t
{
    // From glfw3.h
    Button0 = 0,
    Button1 = 1,
    Button2 = 2,
    Button3 = 3,
    Button4 = 4,
    Button5 = 5,
    Button6 = 6,
    Button7 = 7,

    ButtonLast = Button7,
    ButtonLeft = Button0,
    ButtonRight = Button1,
    ButtonMiddle = Button2
} Mouse;

inline std::ostream& operator<<(std::ostream& os, MouseCode mouseCode)
{
    os << static_cast<int32_t>(mouseCode);
    return os;
}
}

#define BR_MOUSE_BUTTON_0      ::Brigerad::Mouse::Button0
#define BR_MOUSE_BUTTON_1      ::Brigerad::Mouse::Button1
#define BR_MOUSE_BUTTON_2      ::Brigerad::Mouse::Button2
#define BR_MOUSE_BUTTON_3      ::Brigerad::Mouse::Button3
#define BR_MOUSE_BUTTON_4      ::Brigerad::Mouse::Button4
#define BR_MOUSE_BUTTON_5      ::Brigerad::Mouse::Button5
#define BR_MOUSE_BUTTON_6      ::Brigerad::Mouse::Button6
#define BR_MOUSE_BUTTON_7      ::Brigerad::Mouse::Button7
#define BR_MOUSE_BUTTON_LAST   ::Brigerad::Mouse::ButtonLast
#define BR_MOUSE_BUTTON_LEFT   ::Brigerad::Mouse::ButtonLeft
#define BR_MOUSE_BUTTON_RIGHT  ::Brigerad::Mouse::ButtonRight
#define BR_MOUSE_BUTTON_MIDDLE ::Brigerad::Mouse::ButtonMiddle
