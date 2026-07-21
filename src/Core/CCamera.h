//----------------------------------------------------------------------------
//-- Camera.h
//-- VlaGan: Camera class (now just for debug)
//----------------------------------------------------------------------------
#pragma once
#include <DirectXMath.h>
#include <Windows.h>

enum eCameraProjMode {
    ePerspective = 0,
    eOrthographic
};

enum eCameraControlMode {
    eFreeLook = 0,
    eOrbital
};

class CCamera {
public:
    CCamera();
    void Update(float dt);
    void OnMouseMove(int dx, int dy);
    void OnMouseWheel(int delta);

    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMMATRIX GetProjectionMatrix(float aspectRatio) const;
    DirectX::XMFLOAT3 GetPosition() const { return m_position; }
    DirectX::XMFLOAT3 GetTarget() const { return m_target; }

    void SetAspectRatio(float ar) { m_aspectRatio = ar; }
    void SetCameraMode(eCameraProjMode mode) { m_mode = mode; }
    void SetControlMode(eCameraControlMode mode);

    float AspectRatio() { return m_aspectRatio; };
    float OrthoSize() { return m_orthoSize; };

    void SetTarget(const DirectX::XMFLOAT3& target) { m_target = target; }

public:
    //-- projection mode
    eCameraProjMode m_mode; 

    //-- camera control mode
    eCameraControlMode m_controlMode;

    //-- for eOrthographic projection
    float m_orthoSize;

    //-- position / target for orbital
    DirectX::XMFLOAT3 m_position;
    DirectX::XMFLOAT3 m_target;

    //-- orientation
    float m_yaw;
    float m_pitch;

    //-- dist from target to position
    float m_orbitDistance;

    //-- moving speed for freelook
    float m_speed;

    float m_mouseSensitivity;

    //-- fov / aspect
    float m_fov;
    float m_aspectRatio;

    //-- moving camera for freelook
    bool m_moveForward;
    bool m_moveBackward;
    bool m_moveLeft;
    bool m_moveRight;
    bool m_moveUp;
    bool m_moveDown;

    // for orbital
    bool m_MouseMB;
    bool m_LShitft;

    //-- min / max z
    float zNear;
    float zFar;
};
