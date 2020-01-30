#include "camera.hpp"
#include "renderers/render.hpp"

Camera::Camera()
    :
    m_Origin(0.0f, -20.0f, 20.0f),
    m_Pitch(2.0f),
    m_Yaw(MATH_PIDIV2),
    m_Speed(32.0f),
    m_FrameCount(0),
    m_Up(0.0f, 0.0f, 1.0f)
{
    m_Front = float3(cosf(m_Yaw) * sinf(m_Pitch), sinf(m_Yaw) * sinf(m_Pitch), cosf(m_Pitch));
    m_Right = Cross(m_Front, m_Up).Normalize();
    m_Up = Cross(m_Right, m_Front);

    render->GetOCLHelper()->SetArgument(RenderKernelArgument_t::CAM_ORIGIN, &m_Origin, sizeof(float3));
    render->GetOCLHelper()->SetArgument(RenderKernelArgument_t::CAM_FRONT, &m_Front, sizeof(float3));
    render->GetOCLHelper()->SetArgument(RenderKernelArgument_t::CAM_UP, &m_Up, sizeof(float3));
}

void Camera::Update()
{
    render->GetOCLHelper()->SetArgument(RenderKernelArgument_t::FRAME_COUNT, &m_FrameCount, sizeof(unsigned int));

    ++m_FrameCount;
}
