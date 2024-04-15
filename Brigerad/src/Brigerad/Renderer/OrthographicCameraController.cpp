#include "OrthographicCameraController.h"

#include "../Core/Input.h"
#include "../Core/KeyCodes.h"
#include "../Debug/Instrumentor.h"

namespace Brigerad
{

constexpr float MAX_POS_SPEED = 10.0f;
constexpr float MAX_NEG_SPEED = -10.0f;

OrthographicCameraController::OrthographicCameraController(float aspectRatio, bool rotation)
: m_aspectRatio(aspectRatio),
  m_bounds({-m_aspectRatio * m_zoomLevel, m_aspectRatio * m_zoomLevel, -m_zoomLevel, m_zoomLevel}),
  m_camera(-m_aspectRatio * m_zoomLevel, m_aspectRatio * m_zoomLevel, -m_zoomLevel, m_zoomLevel),
  m_allowRotation(rotation)
{
}

void OrthographicCameraController::OnUpdate(Timestep ts)
{
    float ds = (m_cameraTranslationSpeed * ts);
    float dr = (m_cameraRotationSpeed * ts);
    // Down.
    if (Input::isKeyPressed(BR_KEY_S)) { m_cameraPosition.y += ds; }
    // Up.
    if (Input::isKeyPressed(BR_KEY_W)) { m_cameraPosition.y -= ds; }

    // Right.
    if (Input::isKeyPressed(BR_KEY_D)) { m_cameraPosition.x -= ds; }
    // Left.
    if (Input::isKeyPressed(BR_KEY_A)) { m_cameraPosition.x += ds; }

    if (m_allowRotation == true)
    {
        using namespace Brigerad;
        // Tilt CCW.
        if (Input::isKeyPressed(BR_KEY_Q)) { m_cameraRotation += dr; }
        // Tilt CW.
        if (Input::isKeyPressed(BR_KEY_E)) { m_cameraRotation -= dr; }

        m_camera.SetRotation(m_cameraRotation);
    }

    m_camera.SetPosition(m_cameraPosition);

    m_cameraTranslationSpeed = m_zoomLevel;
}

void OrthographicCameraController::OnEvent(Event& e)
{
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<MouseScrolledEvent>(
      BR_BIND_EVENT_FN(OrthographicCameraController::OnMouseScrolled));
    dispatcher.Dispatch<WindowResizeEvent>(
      BR_BIND_EVENT_FN(OrthographicCameraController::OnWindowResized));
}

void OrthographicCameraController::OnResize(float width, float height)
{
    m_aspectRatio = width / height;
    CalculateView();
}


void OrthographicCameraController::CalculateView()
{
    BR_PROFILE_FUNCTION();
    m_bounds = {
      -m_aspectRatio * m_zoomLevel, m_aspectRatio * m_zoomLevel, -m_zoomLevel, m_zoomLevel};
    m_camera.SetProjection(m_bounds.left, m_bounds.right, m_bounds.bottom, m_bounds.top);
}

bool OrthographicCameraController::OnMouseScrolled(MouseScrolledEvent& e)
{
    m_zoomLevel -= e.GetYOffset();
    m_zoomLevel = std::max(m_zoomLevel, 0.25f);
    CalculateView();
    return false;
}

bool OrthographicCameraController::OnWindowResized(WindowResizeEvent& e)
{
    OnResize((float)e.GetWidth(), (float)e.GetHeight());

    return false;
}

}    // namespace Brigerad
