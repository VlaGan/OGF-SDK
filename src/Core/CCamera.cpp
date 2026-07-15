//----------------------------------------------------------------------------
//-- Camera.cpp
//-- VlaGan: Camera class (now just for debug)
//----------------------------------------------------------------------------
#include "CCamera.h"
#include <cmath>
#include <DirectXMath.h>
#include <algorithm>
#include "../Render/_render_structs.h"


CCamera::CCamera() {
    m_position = { 0.f, 0.75f, -5.f };
    m_yaw = 0.f;
    m_pitch = 0.f;
    m_speed = 1.f;
    m_mouseSensitivity = 0.002f;
    m_fov = DirectX::XM_PIDIV4;
    m_aspectRatio = 16.f / 10.f; //-- correct for my laptop, but need autodetect
    m_moveForward = false;
    m_moveBackward = false;
    m_moveLeft = false;
    m_moveRight = false;
    m_moveUp = false;
    m_moveDown = false;

	zNear = 0.1f;
	zFar = 100.f;
}

void CCamera::Update(float dt)
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

    if (m_moveForward)  pos = DirectX::XMVectorAdd(pos,      mul_spdt(forward, m_speed, dt));
    if (m_moveBackward) pos = DirectX::XMVectorSubtract(pos, mul_spdt(forward, m_speed, dt));
    if (m_moveLeft)     pos = DirectX::XMVectorAdd(pos,      mul_spdt(right, m_speed, dt));
    if (m_moveRight)    pos = DirectX::XMVectorSubtract(pos, mul_spdt(right, m_speed, dt));
    if (m_moveUp)       pos = DirectX::XMVectorAdd(pos,      mul_spdt(up, m_speed, dt));
    if (m_moveDown)     pos = DirectX::XMVectorSubtract(pos, mul_spdt(up, m_speed, dt));

    DirectX::XMStoreFloat3(&m_position, pos);
}

void CCamera::OnMouseMove(int dx, int dy)
{
    if (m_mode == eCameraProjMode::Orthographic)
        return;

    m_yaw += dx * m_mouseSensitivity;
    m_pitch -= dy * m_mouseSensitivity;
    m_pitch = std::clamp(m_pitch, -DirectX::XM_PIDIV2 + 0.01f, DirectX::XM_PIDIV2 - 0.01f);
}

void CCamera::OnMouseWheel(int delta)
{
    m_speed += delta * 0.001f;
    m_speed = std::clamp(m_speed, 1.f, 20.f);
}

void CCamera::OnKeyDown(WPARAM key)
{
    if (m_mode == eCameraProjMode::Orthographic)
        return;

    if (key == 'W') m_moveForward = true;
    if (key == 'S') m_moveBackward = true;
    if (key == 'A') m_moveLeft = true;
    if (key == 'D') m_moveRight = true;
    if (key == VK_SPACE) m_moveUp = true;
    if (key == VK_CONTROL) m_moveDown = true;
}

void CCamera::OnKeyUp(WPARAM key)
{
    if (m_mode == eCameraProjMode::Orthographic)
        return;

    if (key == 'W') m_moveForward = false;
    if (key == 'S') m_moveBackward = false;
    if (key == 'A') m_moveLeft = false;
    if (key == 'D') m_moveRight = false;
    if (key == VK_SPACE) m_moveUp = false;
    if (key == VK_CONTROL) m_moveDown = false;
}

DirectX::XMMATRIX CCamera::GetViewMatrix() const
{
    DirectX::XMVECTOR pos = XMLoadFloat3(&m_position);

    DirectX::XMVECTOR forward = DirectX::XMVectorSet(
        cosf(m_pitch) * sinf(m_yaw),
        sinf(m_pitch),
        cosf(m_pitch) * cosf(m_yaw),
        0.f
    );

    DirectX::XMVECTOR target = DirectX::XMVectorAdd(pos, forward);
    DirectX::XMVECTOR up = DirectX::XMVectorSet(0.f, 1.f, 0.f, 0.f);

    return DirectX::XMMatrixLookAtLH(pos, target, up);
}


DirectX::XMMATRIX CCamera::GetProjectionMatrix(float aspectRatio) const
{
	if (m_mode == eCameraProjMode::Orthographic)
    {
        float h = m_orthoSize;
        float w = h * aspectRatio;
        return DirectX::XMMatrixOrthographicLH(w, h, zNear, zFar);
    }

    return DirectX::XMMatrixPerspectiveFovLH(m_fov, aspectRatio, zNear, zFar);
}
