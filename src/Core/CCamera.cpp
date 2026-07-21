//----------------------------------------------------------------------------
//-- Camera.cpp
//-- VlaGan: Camera class (now just for debug)
//----------------------------------------------------------------------------
#include "CCamera.h"
#include <cmath>
#include <DirectXMath.h>
#include <algorithm>
#include "../Render/_render_structs.h"
#include "../_defines.h"

CCamera::CCamera() {

    m_position = { 0.f, 0.75f, -5.f };
    m_target = { 0.f, 0.f, 0.f };

    m_yaw = 0.f;
    m_pitch = DirectX::XM_PIDIV4;

    m_speed = 1.f;
    m_mouseSensitivity = 0.002f;

    m_fov = DirectX::XM_PIDIV4;
    m_aspectRatio = 16.f / 10.f;

    m_moveForward = false;
    m_moveBackward = false;
    m_moveLeft = false;
    m_moveRight = false;
    m_moveUp = false;
    m_moveDown = false;
    m_MouseMB = false;
    m_LShitft = false;

    m_orthoSize = 2.f;
    m_orbitDistance = 5.f;
    m_target = { 0.f, 0.f, 0.f };
    zNear = 0.1f;
    zFar = 100.f;

    m_mode = eCameraProjMode::ePerspective;
    m_controlMode = eCameraControlMode::eOrbital;
}


void CCamera::SetControlMode(eCameraControlMode mode) {
    m_controlMode = mode;

    if (mode == eCameraControlMode::eFreeLook) {
        m_yaw = 0.f;
        m_pitch = 0.f;
        m_position = { 0.f, 0.75f, -5.f };
    }
    else {
        m_target = { 0.f, 0.f, 0.f };
        m_yaw = 0.f;
        m_pitch = DirectX::XM_PIDIV4;
        m_orbitDistance = 5.f;
    }
    m_moveForward = false;
    m_moveBackward = false;
    m_moveLeft = false;
    m_moveRight = false;
    m_moveUp = false;
    m_moveDown = false;
    m_MouseMB = false;
    m_LShitft = false;
}


void CCamera::Update(float dt)
{
    if (m_controlMode == eCameraControlMode::eOrbital)
    {
        float x = m_orbitDistance * cosf(m_pitch) * sinf(m_yaw);
        float y = m_orbitDistance * sinf(m_pitch);
        float z = m_orbitDistance * cosf(m_pitch) * cosf(m_yaw);

        m_position.x = m_target.x + x;
        m_position.y = m_target.y + y;
        m_position.z = m_target.z + z;
    }
    else // Free mode
    {
        DirectX::XMVECTOR forward = DirectX::XMVectorSet(
            cosf(m_pitch) * sinf(m_yaw),
            sinf(m_pitch),
            cosf(m_pitch) * cosf(m_yaw),
            0.f
        );

        DirectX::XMVECTOR right = DirectX::XMVector3Cross(forward, DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f));
        DirectX::XMVECTOR up = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);

        auto mul_spdt = [&](DirectX::XMVECTOR& vec, float& f1, float& f2) {
            DirectX::XMVECTOR res = vec;
            res = DirectX::XMVectorMultiply(res, DirectX::XMVectorSet(f1, f1, f1, f1));
            res = DirectX::XMVectorMultiply(res, DirectX::XMVectorSet(f2, f2, f2, f2));
            return res;
            };

        DirectX::XMVECTOR pos = DirectX::XMLoadFloat3(&m_position);

        if (m_moveForward)  pos = DirectX::XMVectorAdd(pos, mul_spdt(forward, m_speed, dt));
        if (m_moveBackward) pos = DirectX::XMVectorSubtract(pos, mul_spdt(forward, m_speed, dt));
        if (m_moveLeft)     pos = DirectX::XMVectorAdd(pos, mul_spdt(right, m_speed, dt));
        if (m_moveRight)    pos = DirectX::XMVectorSubtract(pos, mul_spdt(right, m_speed, dt));
        if (m_moveUp)       pos = DirectX::XMVectorAdd(pos, mul_spdt(up, m_speed, dt));
        if (m_moveDown)     pos = DirectX::XMVectorSubtract(pos, mul_spdt(up, m_speed, dt));

        DirectX::XMStoreFloat3(&m_position, pos);
    }
}


DirectX::XMMATRIX CCamera::GetViewMatrix() const
{
    DirectX::XMVECTOR pos = XMLoadFloat3(&m_position);
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);

    if (m_controlMode == eCameraControlMode::eOrbital)
    {
        DirectX::XMVECTOR target = XMLoadFloat3(&m_target);
        return DirectX::XMMatrixLookAtLH(pos, target, up);
    }
    else // Free mode
    {
        DirectX::XMVECTOR forward = DirectX::XMVectorSet(
            cosf(m_pitch) * sinf(m_yaw),
            sinf(m_pitch),
            cosf(m_pitch) * cosf(m_yaw),
            0.f
        );
        DirectX::XMVECTOR target = DirectX::XMVectorAdd(pos, forward);
        return DirectX::XMMatrixLookAtLH(pos, target, up);
    }
}


DirectX::XMMATRIX CCamera::GetProjectionMatrix(float aspectRatio) const
{
    if (m_mode == eCameraProjMode::eOrthographic)
    {
        float h = m_orthoSize;
        float w = h * aspectRatio;
        return DirectX::XMMatrixOrthographicLH(w, h, zNear, zFar);
    }

    return DirectX::XMMatrixPerspectiveFovLH(m_fov, aspectRatio, zNear, zFar);
}


//----------------------------------------------------------------------------
//-- Camera transformation on input
//----------------------------------------------------------------------------
void CCamera::OnMouseMove(int dx, int dy)
{
    if (m_controlMode == eCameraControlMode::eOrbital)
    {
        if (m_LShitft && m_MouseMB)
        {
            float panSpeed = 0.002f;
            panSpeed = dxfloat3_dist_to(m_position, m_target) * tan(m_fov / 2) * panSpeed;

            DirectX::XMVECTOR forward = DirectX::XMVectorSet(
                cosf(m_pitch) * sinf(m_yaw),
                sinf(m_pitch),
                cosf(m_pitch) * cosf(m_yaw),
                0.f
            );
            DirectX::XMVECTOR right = DirectX::XMVector3Cross(forward, DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f));
            DirectX::XMVECTOR up = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);

            DirectX::XMVECTOR targetVec = XMLoadFloat3(&m_target);
            targetVec = DirectX::XMVectorAdd(targetVec, DirectX::XMVectorScale(right, -dx * panSpeed));
            targetVec = DirectX::XMVectorAdd(targetVec, DirectX::XMVectorScale(up, dy * panSpeed));
            DirectX::XMStoreFloat3(&m_target, targetVec);
        }
        else if (m_MouseMB)
        {
            m_yaw += dx * m_mouseSensitivity;
            m_pitch += dy * m_mouseSensitivity;
            m_pitch = std::clamp(m_pitch, -DirectX::XM_PIDIV2 + 0.01f, DirectX::XM_PIDIV2 - 0.01f);
        }
    }
    else // Free mode
    {
        if (m_mode == eCameraProjMode::eOrthographic)
            return;

        if (m_MouseMB)
        {
            m_yaw += dx * m_mouseSensitivity;
            m_pitch -= dy * m_mouseSensitivity;
            m_pitch = std::clamp(m_pitch, -DirectX::XM_PIDIV2 + 0.01f, DirectX::XM_PIDIV2 - 0.01f);
        }
    }
}

//-- mouse wheel
void CCamera::OnMouseWheel(int delta)
{
    if (m_controlMode == eCameraControlMode::eOrbital)
    {
        float zoomFactor = 1.0f + -delta * 0.1f; 
        m_orbitDistance *= zoomFactor;
        m_orbitDistance = std::clamp(m_orbitDistance, 0.1f, zFar);
    }
    else // Free mode
    {
        m_speed += delta * 0.001f;
        m_speed = std::clamp(m_speed, 1.f, 20.f);
    }
}
//----------------------------------------------------------------------------
