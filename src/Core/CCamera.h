//----------------------------------------------------------------------------
//-- Camera.h
//-- VlaGan: Camera class (now just for debug)
//----------------------------------------------------------------------------
#pragma once
#include <DirectXMath.h>
#include <Windows.h>

enum eCameraProjMode {
	ePerspective = 0,
    Orthographic
};

class CCamera {
public:
    CCamera();
    void Update(float dt);
    void OnMouseMove(int dx, int dy);
    void OnMouseWheel(int delta);
    void OnKeyDown(WPARAM key);
    void OnKeyUp(WPARAM key);

    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMMATRIX GetProjectionMatrix(float aspectRatio) const;
    DirectX::XMFLOAT3 GetPosition() const { return m_position; }

    void SetAspectRatio(float ar) { m_aspectRatio = ar; }

	void SetCameraMode(eCameraProjMode mode) { m_mode = mode; }
    float AspectRatio() { return m_aspectRatio; };
	float OrthoSize() { return m_orthoSize; };

    public:
    eCameraProjMode m_mode{ eCameraProjMode::ePerspective };
    float m_orthoSize{ 2.0f };

    DirectX::XMFLOAT3 m_position;
    float m_yaw;
    float m_pitch;

    float m_speed;
    float m_mouseSensitivity;
    float m_fov;
    float m_aspectRatio;

    bool m_moveForward;
    bool m_moveBackward;
    bool m_moveLeft;
    bool m_moveRight;
    bool m_moveUp;
    bool m_moveDown;


	float zNear;
	float zFar;
};
