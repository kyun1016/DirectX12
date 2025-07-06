#pragma once
#include "DX12_Config.h"

class CameraSystem
{
public:
	CameraSystem();
	~CameraSystem();

	// Get/Set world camera position.
	float4 GetPosition4f()const;
	float3 GetPosition3f()const;
	void SetPosition(float x, float y, float z);
	void SetPosition(const float3& v);

	// Get camera basis vectors.
	float4 GetRight4f()const;
	float3 GetRight3f()const;
	float4 GetUp4f()const;
	float3 GetUp3f()const;
	float4 GetLook4f()const;
	float3 GetLook3f()const;
	float4 GetQuaternion() const;

	// Get frustum properties.
	float GetNearZ()const;
	float GetFarZ()const;
	float GetAspect()const;
	float GetFovY()const;
	float GetFovX()const;

	// Get near and far plane dimensions in view space coordinates.
	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;

	// Set frustum.
	void SetLens(float fovY, float aspect, float zn, float zf);

	// Define camera space via LookAt parameters.
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);

	// Get View/Proj matrices.
	float4x4 GetView()const;
	float4x4 GetProj()const;

	// Strafe/Walk the camera a distance d.
	void Strafe(float d);
	void Walk(float d);
	void Up(float d);

	// Rotate the camera.
	void Pitch(float angle);
	void RotateY(float angle);

	// After modifying camera position/orientation, call to rebuild the view matrix.
	void UpdateViewMatrix();

private:

	// Camera coordinate system with coordinates relative to world space.
	float3 mPosition = { 0.0f, 0.0f, 0.0f };
	float3 mRight = { 1.0f, 0.0f, 0.0f };
	float3 mUp = { 0.0f, 1.0f, 0.0f };
	float3 mLook = { 0.0f, 0.0f, 1.0f };

	// Cache frustum properties.
	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	bool mViewDirty = true;

	float mSpeed = 3.0f;

	// Cache View/Proj matrices.
	float4x4 mView;
	float4x4 mProj;
};