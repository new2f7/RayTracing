#ifndef CAMERA_HPP
#define CAMERA_HPP

#include "mathlib/mathlib.hpp"
#include "utils/viewport.hpp"
#include <memory>

class Camera
{
public:
    Camera();

    void Update();

    unsigned int GetFrameCount() const { return m_FrameCount; }

private:
    float3 m_Origin;
    float3 m_Front;
    float3 m_Up;
    float3 m_Right;

    float m_Pitch;
    float m_Yaw;    
    float m_Speed;

    unsigned int m_FrameCount;

};

#endif // CAMERA_HPP
