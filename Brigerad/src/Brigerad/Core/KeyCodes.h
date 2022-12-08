#pragma once

namespace Brigerad
{
typedef enum class KeyCode : uint16_t
{
    // From glfw3.h
    Space = 32,
    Apostrophe = 39, /* ' */
    Comma = 44, /* , */
    Minus = 45, /* - */
    Period = 46, /* . */
    Slash = 47, /* / */

    D0 = 48, /* 0 */
    D1 = 49, /* 1 */
    D2 = 50, /* 2 */
    D3 = 51, /* 3 */
    D4 = 52, /* 4 */
    D5 = 53, /* 5 */
    D6 = 54, /* 6 */
    D7 = 55, /* 7 */
    D8 = 56, /* 8 */
    D9 = 57, /* 9 */

    Semicolon = 59, /* ; */
    Equal = 61, /* = */

    A = 65,
    B = 66,
    C = 67,
    D = 68,
    E = 69,
    F = 70,
    G = 71,
    H = 72,
    I = 73,
    J = 74,
    K = 75,
    L = 76,
    M = 77,
    N = 78,
    O = 79,
    P = 80,
    Q = 81,
    R = 82,
    S = 83,
    T = 84,
    U = 85,
    V = 86,
    W = 87,
    X = 88,
    Y = 89,
    Z = 90,

    LeftBracket = 91,  /* [ */
    Backslash = 92,  /* \ */
    RightBracket = 93,  /* ] */
    GraveAccent = 96,  /* ` */

    World1 = 161, /* non-US #1 */
    World2 = 162, /* non-US #2 */

    /* Function keys */
    Escape = 256,
    Enter = 257,
    Tab = 258,
    Backspace = 259,
    Insert = 260,
    Delete = 261,
    Right = 262,
    Left = 263,
    Down = 264,
    Up = 265,
    PageUp = 266,
    PageDown = 267,
    Home = 268,
    End = 269,
    CapsLock = 280,
    ScrollLock = 281,
    NumLock = 282,
    PrintScreen = 283,
    Pause = 284,
    F1 = 290,
    F2 = 291,
    F3 = 292,
    F4 = 293,
    F5 = 294,
    F6 = 295,
    F7 = 296,
    F8 = 297,
    F9 = 298,
    F10 = 299,
    F11 = 300,
    F12 = 301,
    F13 = 302,
    F14 = 303,
    F15 = 304,
    F16 = 305,
    F17 = 306,
    F18 = 307,
    F19 = 308,
    F20 = 309,
    F21 = 310,
    F22 = 311,
    F23 = 312,
    F24 = 313,
    F25 = 314,

    /* Keypad */
    KP0 = 320,
    KP1 = 321,
    KP2 = 322,
    KP3 = 323,
    KP4 = 324,
    KP5 = 325,
    KP6 = 326,
    KP7 = 327,
    KP8 = 328,
    KP9 = 329,
    KPDecimal = 330,
    KPDivide = 331,
    KPMultiply = 332,
    KPSubtract = 333,
    KPAdd = 334,
    KPEnter = 335,
    KPEqual = 336,

    LeftShift = 340,
    LeftControl = 341,
    LeftAlt = 342,
    LeftSuper = 343,
    RightShift = 344,
    RightControl = 345,
    RightAlt = 346,
    RightSuper = 347,
    Menu = 348
} Key;

inline std::ostream& operator<<(std::ostream& os, KeyCode keyCode)
{
    os << static_cast<int32_t>(keyCode);
    return os;
}
}

// From glfw3.h
#define BR_KEY_SPACE           ::Brigerad::Key::Space
#define BR_KEY_APOSTROPHE      ::Brigerad::Key::Apostrophe    /* ' */
#define BR_KEY_COMMA           ::Brigerad::Key::Comma         /* , */
#define BR_KEY_MINUS           ::Brigerad::Key::Minus         /* - */
#define BR_KEY_PERIOD          ::Brigerad::Key::Period        /* . */
#define BR_KEY_SLASH           ::Brigerad::Key::Slash         /* / */
#define BR_KEY_0               ::Brigerad::Key::D0
#define BR_KEY_1               ::Brigerad::Key::D1
#define BR_KEY_2               ::Brigerad::Key::D2
#define BR_KEY_3               ::Brigerad::Key::D3
#define BR_KEY_4               ::Brigerad::Key::D4
#define BR_KEY_5               ::Brigerad::Key::D5
#define BR_KEY_6               ::Brigerad::Key::D6
#define BR_KEY_7               ::Brigerad::Key::D7
#define BR_KEY_8               ::Brigerad::Key::D8
#define BR_KEY_9               ::Brigerad::Key::D9
#define BR_KEY_SEMICOLON       ::Brigerad::Key::Semicolon     /* ; */
#define BR_KEY_EQUAL           ::Brigerad::Key::Equal         /* = */
#define BR_KEY_A               ::Brigerad::Key::A
#define BR_KEY_B               ::Brigerad::Key::B
#define BR_KEY_C               ::Brigerad::Key::C
#define BR_KEY_D               ::Brigerad::Key::D
#define BR_KEY_E               ::Brigerad::Key::E
#define BR_KEY_F               ::Brigerad::Key::F
#define BR_KEY_G               ::Brigerad::Key::G
#define BR_KEY_H               ::Brigerad::Key::H
#define BR_KEY_I               ::Brigerad::Key::I
#define BR_KEY_J               ::Brigerad::Key::J
#define BR_KEY_K               ::Brigerad::Key::K
#define BR_KEY_L               ::Brigerad::Key::L
#define BR_KEY_M               ::Brigerad::Key::M
#define BR_KEY_N               ::Brigerad::Key::N
#define BR_KEY_O               ::Brigerad::Key::O
#define BR_KEY_P               ::Brigerad::Key::P
#define BR_KEY_Q               ::Brigerad::Key::Q
#define BR_KEY_R               ::Brigerad::Key::R
#define BR_KEY_S               ::Brigerad::Key::S
#define BR_KEY_T               ::Brigerad::Key::T
#define BR_KEY_U               ::Brigerad::Key::U
#define BR_KEY_V               ::Brigerad::Key::V
#define BR_KEY_W               ::Brigerad::Key::W
#define BR_KEY_X               ::Brigerad::Key::X
#define BR_KEY_Y               ::Brigerad::Key::Y
#define BR_KEY_Z               ::Brigerad::Key::Z
#define BR_KEY_LEFT_BRACKET    ::Brigerad::Key::LeftBracket   /* [ */
#define BR_KEY_BACKSLASH       ::Brigerad::Key::Backslash     /* \ */
#define BR_KEY_RIGHT_BRACKET   ::Brigerad::Key::RightBracket  /* ] */
#define BR_KEY_GRAVE_ACCENT    ::Brigerad::Key::GraveAccent   /* ` */
#define BR_KEY_WORLD_1         ::Brigerad::Key::World1        /* non-US #1 */
#define BR_KEY_WORLD_2         ::Brigerad::Key::World2        /* non-US #2 */

/* Function keys */
#define BR_KEY_ESCAPE          ::Brigerad::Key::Escape
#define BR_KEY_ENTER           ::Brigerad::Key::Enter
#define BR_KEY_TAB             ::Brigerad::Key::Tab
#define BR_KEY_BACKSPACE       ::Brigerad::Key::Backspace
#define BR_KEY_INSERT          ::Brigerad::Key::Insert
#define BR_KEY_DELETE          ::Brigerad::Key::Delete
#define BR_KEY_RIGHT           ::Brigerad::Key::Right
#define BR_KEY_LEFT            ::Brigerad::Key::Left
#define BR_KEY_DOWN            ::Brigerad::Key::Down
#define BR_KEY_UP              ::Brigerad::Key::Up
#define BR_KEY_PAGE_UP         ::Brigerad::Key::PageUp
#define BR_KEY_PAGE_DOWN       ::Brigerad::Key::PageDown
#define BR_KEY_HOME            ::Brigerad::Key::Home
#define BR_KEY_END             ::Brigerad::Key::End
#define BR_KEY_CAPS_LOCK       ::Brigerad::Key::CapsLock
#define BR_KEY_SCROLL_LOCK     ::Brigerad::Key::ScrollLock
#define BR_KEY_NUM_LOCK        ::Brigerad::Key::NumLock
#define BR_KEY_PRINT_SCREEN    ::Brigerad::Key::PrintScreen
#define BR_KEY_PAUSE           ::Brigerad::Key::Pause
#define BR_KEY_F1              ::Brigerad::Key::F1
#define BR_KEY_F2              ::Brigerad::Key::F2
#define BR_KEY_F3              ::Brigerad::Key::F3
#define BR_KEY_F4              ::Brigerad::Key::F4
#define BR_KEY_F5              ::Brigerad::Key::F5
#define BR_KEY_F6              ::Brigerad::Key::F6
#define BR_KEY_F7              ::Brigerad::Key::F7
#define BR_KEY_F8              ::Brigerad::Key::F8
#define BR_KEY_F9              ::Brigerad::Key::F9
#define BR_KEY_F10             ::Brigerad::Key::F10
#define BR_KEY_F11             ::Brigerad::Key::F11
#define BR_KEY_F12             ::Brigerad::Key::F12
#define BR_KEY_F13             ::Brigerad::Key::F13
#define BR_KEY_F14             ::Brigerad::Key::F14
#define BR_KEY_F15             ::Brigerad::Key::F15
#define BR_KEY_F16             ::Brigerad::Key::F16
#define BR_KEY_F17             ::Brigerad::Key::F17
#define BR_KEY_F18             ::Brigerad::Key::F18
#define BR_KEY_F19             ::Brigerad::Key::F19
#define BR_KEY_F20             ::Brigerad::Key::F20
#define BR_KEY_F21             ::Brigerad::Key::F21
#define BR_KEY_F22             ::Brigerad::Key::F22
#define BR_KEY_F23             ::Brigerad::Key::F23
#define BR_KEY_F24             ::Brigerad::Key::F24
#define BR_KEY_F25             ::Brigerad::Key::F25

/* Keypad */
#define BR_KEY_KP_0            ::Brigerad::Key::KP0
#define BR_KEY_KP_1            ::Brigerad::Key::KP1
#define BR_KEY_KP_2            ::Brigerad::Key::KP2
#define BR_KEY_KP_3            ::Brigerad::Key::KP3
#define BR_KEY_KP_4            ::Brigerad::Key::KP4
#define BR_KEY_KP_5            ::Brigerad::Key::KP5
#define BR_KEY_KP_6            ::Brigerad::Key::KP6
#define BR_KEY_KP_7            ::Brigerad::Key::KP7
#define BR_KEY_KP_8            ::Brigerad::Key::KP8
#define BR_KEY_KP_9            ::Brigerad::Key::KP9
#define BR_KEY_KP_DECIMAL      ::Brigerad::Key::KPDecimal
#define BR_KEY_KP_DIVIDE       ::Brigerad::Key::KPDivide
#define BR_KEY_KP_MULTIPLY     ::Brigerad::Key::KPMultiply
#define BR_KEY_KP_SUBTRACT     ::Brigerad::Key::KPSubtract
#define BR_KEY_KP_ADD          ::Brigerad::Key::KPAdd
#define BR_KEY_KP_ENTER        ::Brigerad::Key::KPEnter
#define BR_KEY_KP_EQUAL        ::Brigerad::Key::KPEqual

#define BR_KEY_LEFT_SHIFT      ::Brigerad::Key::LeftShift
#define BR_KEY_LEFT_CONTROL    ::Brigerad::Key::LeftControl
#define BR_KEY_LEFT_ALT        ::Brigerad::Key::LeftAlt
#define BR_KEY_LEFT_SUPER      ::Brigerad::Key::LeftSuper
#define BR_KEY_RIGHT_SHIFT     ::Brigerad::Key::RightShift
#define BR_KEY_RIGHT_CONTROL   ::Brigerad::Key::RightControl
#define BR_KEY_RIGHT_ALT       ::Brigerad::Key::RightAlt
#define BR_KEY_RIGHT_SUPER     ::Brigerad::Key::RightSuper
#define BR_KEY_MENU            ::Brigerad::Key::Menu
