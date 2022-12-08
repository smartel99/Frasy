#pragma once
#include "Brigerad/Core/Timestep.h"
#include "Brigerad/Events/ApplicationEvent.h"
#include "Brigerad/Events/MouseEvent.h"
#include "Brigerad/Renderer/OrthographicCamera.h"

namespace Brigerad
{
struct OrthographicCameraBounds
{
    float left, right;
    float bottom, top;

    float GetWidth()
    {
        return right - left;
    }
    float GetHeight()
    {
        return top - bottom;
    }
};

class OrthographicCameraController
{
public:
    OrthographicCameraController(float aspectRatio, bool rotation = false);

    void OnUpdate(Timestep ts);
    void OnEvent(Event& e);

    void OnResize(float width, float height);

    OrthographicCamera& GetCamera()
    {
        return m_camera;
    }
    const OrthographicCamera& GetCamera() const
    {
        return m_camera;
    }

    inline void SetZoomLevel(float level)
    {
        m_zoomLevel = level;
    }
    inline float GetZoomLevel() const
    {
        return m_zoomLevel;
    }

    const OrthographicCameraBounds& GetBounds() const
    {
        return m_bounds;
    }

private:
    void CalculateView();
    bool OnMouseScrolled(MouseScrolledEvent& e);
    bool OnWindowResized(WindowResizeEvent& e);

private:
    float m_aspectRatio;
    float m_zoomLevel = 0.5f;
    OrthographicCamera m_camera;
    OrthographicCameraBounds m_bounds;

    bool m_allowRotation = false;
    glm::vec3 m_cameraPosition = { 0.0f, 0.0f, 0.0f };
    float m_cameraRotation = 0.0f;

    float m_cameraTranslationSpeed = 10.0f;
    float m_cameraRotationSpeed = 180.0f;
};

}  // namespace Brigerad