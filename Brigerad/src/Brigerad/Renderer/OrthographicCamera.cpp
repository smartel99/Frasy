/**
 * @file   E:\dev\Brigerad\Brigerad\src\Brigerad\Renderer\OrthographicCamera.cpp
 * @author Samuel Martel
 * @date   2020/04/04
 *
 * @brief  Source for the OrthographicCamera module.
 */
#include "OrthographicCamera.h"

#include "../Debug/Instrumentor.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Brigerad
{

OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
: m_projectionMatrix(glm::ortho(left, right, bottom, top, -1.0f, 1.0f)), m_viewMatrix(1.0f)
{
    BR_PROFILE_FUNCTION();

    m_vpMatrix = m_projectionMatrix * m_viewMatrix;
}

void OrthographicCamera::SetProjection(float left, float right, float bottom, float top)
{
    BR_PROFILE_FUNCTION();

    m_projectionMatrix = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
    m_vpMatrix         = m_projectionMatrix * m_viewMatrix;
}

void OrthographicCamera::RecalculateViewMatrix()
{
    BR_PROFILE_FUNCTION();

    glm::mat4 transform =
      glm::translate(glm::mat4(1.0f), m_position) *
      glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation), glm::vec3(0, 0, 1));

    m_viewMatrix = glm::inverse(transform);
    m_vpMatrix   = m_projectionMatrix * m_viewMatrix;
}

}    // namespace Brigerad
