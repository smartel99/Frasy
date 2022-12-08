
/**
 * @file   E:\dev\Brigerad\Brigerad\src\Platform\Linux\LinuxInput.cpp
 * @author Samuel Martel
 * @date   2020/03/01
 *
 * @brief  Source for the LinuxInput module.
 */
#include "brpch.h"
#include "Brigerad/Core/Input.h"
#include "Brigerad/Core/Application.h"

#if defined(BR_PLATFORM_LINUX)
#include <GLFW/glfw3.h>

namespace Brigerad
{

bool Input::IsKeyPressed(KeyCode keycode)
{
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto state  = glfwGetKey(window, int(keycode));

    return (state == GLFW_PRESS || state == GLFW_REPEAT);
}

bool Input::IsMouseButtonPressed(MouseCode button)
{
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto state  = glfwGetMouseButton(window, int(button));

    return state == GLFW_PRESS;
}

std::pair<float, float> Input::GetMousePos()
{
    auto   window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);

    return {(float)xPos, (float)yPos};
}

float Input::GetMouseX()
{
    auto [x, y] = GetMousePos();

    return x;
}

float Input::GetMouseY()
{
    auto [x, y] = GetMousePos();

    return y;
}

}    // namespace Brigerad
#endif
