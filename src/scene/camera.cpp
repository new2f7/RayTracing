#include "camera.hpp"
#include "renderers/render.hpp"
#include <iostream>

Camera::Camera(std::shared_ptr<Viewport> viewport)
    :
    m_Viewport(viewport),
    m_Origin(0.0f, -20.0f, 20.0f),
    m_Pitch(2.0f),
    m_Yaw(MATH_PIDIV2),
    m_Speed(32.0f),
    m_FrameCount(0),
    m_Up(0.0f, 0.0f, 1.0f)
{
}

void Camera::Update()
{
    int frontback = 0;
    int strafe = 0;

    if (frontback != 0 || strafe != 0)
    {
        m_FrameCount = 0;
    }
    
    m_Front = float3(cosf(m_Yaw) * sinf(m_Pitch), sinf(m_Yaw) * sinf(m_Pitch), cosf(m_Pitch));

    float3 right = Cross(m_Front, m_Up).Normalize();
    float3 up = Cross(right, m_Front);

    render->GetCLKernel()->SetArgument(RenderKernelArgument_t::CAM_ORIGIN, &m_Origin, sizeof(float3));
    render->GetCLKernel()->SetArgument(RenderKernelArgument_t::CAM_FRONT, &m_Front, sizeof(float3));
    render->GetCLKernel()->SetArgument(RenderKernelArgument_t::CAM_UP, &up, sizeof(float3));
    render->GetCLKernel()->SetArgument(RenderKernelArgument_t::FRAME_COUNT, &m_FrameCount, sizeof(unsigned int));
    unsigned int seed = rand();
    render->GetCLKernel()->SetArgument(RenderKernelArgument_t::FRAME_SEED, &seed, sizeof(unsigned int));
    
    ++m_FrameCount;

}
