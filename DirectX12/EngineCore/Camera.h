#pragma once

#include "SimpleMath.h"

class Camera {
public:
	Camera();

	DirectX::SimpleMath::Matrix GetViewRow();
	DirectX::SimpleMath::Matrix GetProjRow();
	DirectX::SimpleMath::Vector3 GetEyePos();

	void Reset(DirectX::SimpleMath::Vector3 pos, float yaw, float pitch) {
		m_position = pos;
		m_yaw = yaw;
		m_pitch = pitch;
		UpdateViewDir();
	}

	void UpdateViewDir();
	void UpdateKeyboard(const float dt, bool const keyPressed[256]);
	void UpdateMouse(float mouseNdcX, float mouseNdcY);
	void MoveForward(float dt);
	void MoveRight(float dt);
	void MoveUp(float dt);
	void SetAspectRatio(float aspect);
	void PrintView();

public:
	bool m_useFirstPersonView = false;

private:
	DirectX::SimpleMath::Vector3 m_position = DirectX::SimpleMath::Vector3(0.312183f, 0.957463f, -1.88458f);
	DirectX::SimpleMath::Vector3 m_viewDir = DirectX::SimpleMath::Vector3(0.0f, 0.0f, 1.0f);
	DirectX::SimpleMath::Vector3 m_upDir = DirectX::SimpleMath::Vector3(0.0f, 1.0f, 0.0f); // +Y 방향으로 고정
	DirectX::SimpleMath::Vector3 m_rightDir = DirectX::SimpleMath::Vector3(1.0f, 0.0f, 0.0f);

	// roll, pitch, yaw
	// https://en.wikipedia.org/wiki/Aircraft_principal_axes
	float m_yaw = -0.0589047f, m_pitch = 0.213803f;

	float m_speed = 3.0f; // 움직이는 속도

	// 프로젝션 옵션도 카메라 클래스로 이동
	float m_projFovAngleY = 90.0f * 0.5f; // Luna 교재 기본 설정
	float m_nearZ = 0.01f;
	float m_farZ = 100.0f;
	float m_aspect = 16.0f / 9.0f;
	bool m_usePerspectiveProjection = true;
};