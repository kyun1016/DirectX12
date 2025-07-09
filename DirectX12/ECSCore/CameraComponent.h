#pragma once
#include "CameraData.h"

struct CameraComponent
{
	//float FieldOfView = XM_PIDIV4;
	//float3 w_Position = { 0.0f, 0.0f, 0.0f };
	//float NearZ = 0.0f;
	//float3 w_Right = { 1.0f, 0.0f, 0.0f };
	//float FarZ = 0.0f;
	//float3 w_Up = { 0.0f, 1.0f, 0.0f };
	//float Aspect = 0.0f;
	//float3 w_Look = { 0.0f, 0.0f, 1.0f };
	//float FovY = 0.0f;
	//float3 r_Position = { 0.0f, 0.0f, 0.0f };
	//float NearWindowHeight = 0.0f;
	//float3 r_Right = { 1.0f, 0.0f, 0.0f };
	//float FarWindowHeight = 0.0f;
	//float3 r_Up = { 0.0f, 1.0f, 0.0f };
	//float Speed = 3.0f;
	//float3 r_Look = { 0.0f, 0.0f, 1.0f };
	//bool Dirty = true;

	CameraData CameraData;
	DirectX::BoundingFrustum Frustum;

	bool Dirty = true;
	uint32_t NumFramesDirty = APP_NUM_BACK_BUFFERS;
	
	float DefaultFOV = DirectX::XM_PIDIV4;
	float CurrentFOV = XM_PIDIV4;
	float MovementSpeed = 3.0f;
	float RotationSpeed = 0.5f;
	float NearZ = 1.0f;
	float FarZ = 100.0f;
	float Aspect = 1.0f;
	float NearWindowHeight = 0.0f;
	float FarWindowHeight = 0.0f;
	float PitchDelta = 0.0f;
	float YawDelta = 0.0f;
	float3 MovementDelta = { 0.0f, 0.0f, 0.0f };
	
	float3 w_Position = { 0.0f, 0.0f, 0.0f };
	float3 w_Right = { 1.0f, 0.0f, 0.0f };
	float3 w_Up = { 0.0f, 1.0f, 0.0f };
	float3 w_Look = { 0.0f, 0.0f, 1.0f };
	float3 r_Position = { 0.0f, 0.0f, 0.0f };
	float3 r_Right = { 1.0f, 0.0f, 0.0f };
	float3 r_Up = { 0.0f, 1.0f, 0.0f };
	float3 r_Look = { 0.0f, 0.0f, 1.0f };
public:
	void SyncData() {
		if (PitchDelta || YawDelta || MovementDelta.x || MovementDelta.y || MovementDelta.x)
		{
			Dirty = true;
		}

		if (!Dirty)
			return;

		Dirty = false;
		NumFramesDirty = APP_NUM_BACK_BUFFERS;
		// Step 1. Update rotation
		DirectX::XMVECTOR look = DirectX::XMLoadFloat3(&w_Look);
		DirectX::XMVECTOR right = DirectX::XMLoadFloat3(&w_Right);
		DirectX::XMVECTOR up = DirectX::XMLoadFloat3(&w_Up);

		// 1.1. yaw 회전
		float fovScale = CurrentFOV / DefaultFOV;
		DirectX::XMMATRIX yawMatrix = DirectX::XMMatrixRotationY(YawDelta * RotationSpeed * fovScale);
		look = DirectX::XMVector3TransformNormal(look, yawMatrix);
		right = DirectX::XMVector3TransformNormal(right, yawMatrix);
		up = DirectX::XMVector3TransformNormal(up, yawMatrix);
		// 1.2. pitch 회전
		DirectX::XMMATRIX pitchMatrix = DirectX::XMMatrixRotationAxis(right, PitchDelta * RotationSpeed * fovScale);
		look = DirectX::XMVector3TransformNormal(look, pitchMatrix);
		up = DirectX::XMVector3TransformNormal(up, pitchMatrix);
		// 1.3. Re-orthogonalize the vectors
		up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(look, right));
		right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, look));
		// 1.4. Store the results
		DirectX::XMStoreFloat3(&r_Right, right);
		DirectX::XMStoreFloat3(&r_Up, up);
		DirectX::XMStoreFloat3(&r_Look, look);

		// Step 2. Update position
		DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&w_Position);
		position = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorReplicate(MovementDelta.z * MovementSpeed), DirectX::XMLoadFloat3(&r_Look), position);
		position = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorReplicate(MovementDelta.x * MovementSpeed), DirectX::XMLoadFloat3(&r_Right), position);
		position = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorReplicate(MovementDelta.y * MovementSpeed), DirectX::XMLoadFloat3(&r_Up), position);
		DirectX::XMStoreFloat3(&r_Position, position);

		// Step 3. Update Write Buffer
		w_Position = r_Position;
		w_Look = r_Look;
		w_Right = r_Right;
		w_Up = r_Up;


		// Step 4. Update View/Proj matrices (이 CameraData는 SceneSystem에서 추가적으로 응용하며 GPU메모리 업로드를 관리함)
		float x = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(position, right));
		float y = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(position, up));
		float z = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(position, look));

		CameraData.View(0, 0) = r_Right.x;
		CameraData.View(1, 0) = r_Right.y;
		CameraData.View(2, 0) = r_Right.z;
		CameraData.View(3, 0) = x;

		CameraData.View(0, 1) = r_Up.x;
		CameraData.View(1, 1) = r_Up.y;
		CameraData.View(2, 1) = r_Up.z;
		CameraData.View(3, 1) = y;

		CameraData.View(0, 2) = r_Look.x;
		CameraData.View(1, 2) = r_Look.y;
		CameraData.View(2, 2) = r_Look.z;
		CameraData.View(3, 2) = z;

		CameraData.View(0, 3) = 0.0f;
		CameraData.View(1, 3) = 0.0f;
		CameraData.View(2, 3) = 0.0f;
		CameraData.View(3, 3) = 1.0f;

		CameraData.Proj = DirectX::XMMatrixPerspectiveFovLH(CurrentFOV, Aspect, NearZ, FarZ);

		CameraData.ViewProj = DirectX::XMMatrixMultiply(CameraData.View, CameraData.Proj);
	}

	float4 GetPosition4f()const { return float4(r_Position.x, r_Position.y, r_Position.z, 0.0f); }
	float3 GetPosition3f()const { return r_Position; }
	float4 GetRight4f()const { return float4(r_Right.x, r_Right.y, r_Right.z, 0.0f); }
	float3 GetRight3f()const { return r_Right; }
	float4 GetUp4f()const { return float4(r_Up.x, r_Up.y, r_Up.z, 0.0f); }
	float3 GetUp3f()const { return r_Up; }
	float4 GetLook4f()const { return float4(r_Look.x, r_Look.y, r_Look.z, 0.0f); }
	float3 GetLook3f()const { return r_Look; }
	float4 GetQuaternion() const {
		DirectX::SimpleMath::Quaternion q;

		float trace = r_Right.x + r_Up.y + r_Look.z;
		if (trace > 0.0f) {
			float s = 0.5f / sqrt(trace + 1.0f);
			q.w = 0.25f / s;
			q.x = (r_Up.z - r_Look.y) * s;
			q.y = (r_Look.x - r_Right.z) * s;
			q.z = (r_Right.y - r_Up.x) * s;
		}
		else {
			if (r_Right.x > r_Up.y && r_Right.x > r_Look.z) {
				float s = 2.0f * sqrt(1.0f + r_Right.x - r_Up.y - r_Look.z);
				q.w = (r_Up.z - r_Look.y) / s;
				q.x = 0.25f * s;
				q.y = (r_Up.x + r_Right.y) / s;
				q.z = (r_Look.x + r_Right.z) / s;
			}
			else if (r_Up.y > r_Look.z) {
				float s = 2.0f * sqrt(1.0f + r_Up.y - r_Right.x - r_Look.z);
				q.w = (r_Look.x - r_Right.z) / s;
				q.x = (r_Up.x + r_Right.y) / s;
				q.y = 0.25f * s;
				q.z = (r_Look.y + r_Up.z) / s;
			}
			else {
				float s = 2.0f * sqrt(1.0f + r_Look.z - r_Right.x - r_Up.y);
				q.w = (r_Right.y - r_Up.x) / s;
				q.x = (r_Look.x + r_Right.z) / s;
				q.y = (r_Look.y + r_Up.z) / s;
				q.z = 0.25f * s;
			}
		}
		return q;
	}
	float GetNearZ()const { return NearZ; }
	float GetFarZ()const { return FarZ; }
	float GetAspect()const { return Aspect; }
	float GetFovY()const { return CurrentFOV; }
	float GetFovX()const {
		float halfWidth = 0.5f * GetNearWindowWidth();
		return 2.0f * atan(halfWidth / NearZ);
	}
	float GetNearWindowWidth()const { return Aspect * NearWindowHeight; }
	float GetNearWindowHeight()const { return NearWindowHeight; }
	float GetFarWindowWidth()const { return Aspect * FarWindowHeight; }
	float GetFarWindowHeight()const { return FarWindowHeight; }

	// Set frustum.
	void SetPosition(float x, float y, float z) {
		Dirty = true;
		w_Position = { x, y, z };
	}
	void SetPosition(const float3& v) {
		Dirty = true;
		w_Position = v;
	}
	void SetMovementSpeed(float speed) { MovementSpeed = speed; }
	void SetRotationSpeed(float speed) { RotationSpeed = speed; }
	void SetLens(float fovY, float aspect, float nearZ, float farZ)
	{
		Dirty = true;
		CurrentFOV = fovY;
		Aspect = aspect;
		NearZ = nearZ;
		FarZ = farZ;
		NearWindowHeight = 2.0f * nearZ * tanf(0.5f * fovY);
		FarWindowHeight = 2.0f * farZ * tanf(0.5f * fovY);
	}
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp)
	{
		Dirty = true;
		DirectX::XMVECTOR L = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(target, pos));
		DirectX::XMVECTOR R = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(worldUp, L));
		DirectX::XMVECTOR U = DirectX::XMVector3Cross(L, R);

		DirectX::XMStoreFloat3(&w_Position, pos);
		DirectX::XMStoreFloat3(&w_Look, L);
		DirectX::XMStoreFloat3(&w_Right, R);
		DirectX::XMStoreFloat3(&w_Up, U);
	}
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up)
	{
		DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&pos);
		DirectX::XMVECTOR T = DirectX::XMLoadFloat3(&target);
		DirectX::XMVECTOR U = DirectX::XMLoadFloat3(&up);

		LookAt(P, T, U);
	}
	void Strafe(float d) { MovementDelta.x += d; }
	void Walk(float d) { MovementDelta.z += d; }
	void Up(float d) { MovementDelta.y += d; }
	void Pitch(float angle) { PitchDelta += angle; }
	void RotateY(float angle) { YawDelta += angle; }
};

//class Camera
//{
//public:
//	void SyncData() {
//		if (component.PitchDelta || component.YawDelta || component.MovementDelta.x || component.MovementDelta.y || component.MovementDelta.x)
//		{
//			component.Dirty = true;
//		}
//
//		if (!component.Dirty)
//			return;
//
//		// Step 1. Update rotation
//		DirectX::XMVECTOR look = DirectX::XMLoadFloat3(&component.w_Look);
//		DirectX::XMVECTOR right = DirectX::XMLoadFloat3(&component.w_Right);
//		DirectX::XMVECTOR up = DirectX::XMLoadFloat3(&component.w_Up);
//
//		// 1.1. yaw 회전
//		float fovScale = component.CurrentFOV / component.DefaultFOV;
//		DirectX::XMMATRIX yawMatrix = DirectX::XMMatrixRotationY(component.YawDelta * component.RotationSpeed * fovScale);
//		look = DirectX::XMVector3TransformNormal(look, yawMatrix);
//		right = DirectX::XMVector3TransformNormal(right, yawMatrix);
//		up = DirectX::XMVector3TransformNormal(up, yawMatrix);
//		// 1.2. pitch 회전
//		DirectX::XMMATRIX pitchMatrix = DirectX::XMMatrixRotationAxis(right, component.PitchDelta * component.RotationSpeed * fovScale);
//		look = DirectX::XMVector3TransformNormal(look, pitchMatrix);
//		up = DirectX::XMVector3TransformNormal(up, pitchMatrix);
//		// 1.3. Re-orthogonalize the vectors
//		up = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(look, right));
//		right = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(up, look));
//		// 1.4. Store the results
//		DirectX::XMStoreFloat3(&component.r_Right, right);
//		DirectX::XMStoreFloat3(&component.r_Up, up);
//		DirectX::XMStoreFloat3(&component.r_Look, look);
//
//		// Step 2. Update position
//		DirectX::XMVECTOR position = DirectX::XMLoadFloat3(&component.w_Position);
//		position = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorReplicate(component.MovementDelta.z * component.MovementSpeed), DirectX::XMLoadFloat3(&component.r_Look), position);
//		position = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorReplicate(component.MovementDelta.x * component.MovementSpeed), DirectX::XMLoadFloat3(&component.r_Right), position);
//		position = DirectX::XMVectorMultiplyAdd(DirectX::XMVectorReplicate(component.MovementDelta.y * component.MovementSpeed), DirectX::XMLoadFloat3(&component.r_Up), position);
//		DirectX::XMStoreFloat3(&component.r_Position, position);
//
//		// Step 3. Update Write Buffer
//		component.w_Position = component.r_Position;
//		component.w_Look = component.r_Look;
//		component.w_Right = component.r_Right;
//		component.w_Up = component.r_Up;
//
//
//		// Step 4. Update View/Proj matrices (이 CameraData는 SceneSystem에서 추가적으로 응용하며 GPU메모리 업로드를 관리함)
//		float x = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(position, right));
//		float y = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(position, up));
//		float z = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(position, look));
//
//		component.CameraData.View(0, 0) = component.r_Right.x;
//		component.CameraData.View(1, 0) = component.r_Right.y;
//		component.CameraData.View(2, 0) = component.r_Right.z;
//		component.CameraData.View(3, 0) = x;
//
//		component.CameraData.View(0, 1) = component.r_Up.x;
//		component.CameraData.View(1, 1) = component.r_Up.y;
//		component.CameraData.View(2, 1) = component.r_Up.z;
//		component.CameraData.View(3, 1) = y;
//
//		component.CameraData.View(0, 2) = component.r_Look.x;
//		component.CameraData.View(1, 2) = component.r_Look.y;
//		component.CameraData.View(2, 2) = component.r_Look.z;
//		component.CameraData.View(3, 2) = z;
//
//		component.CameraData.View(0, 3) = 0.0f;
//		component.CameraData.View(1, 3) = 0.0f;
//		component.CameraData.View(2, 3) = 0.0f;
//		component.CameraData.View(3, 3) = 1.0f;
//
//		component.CameraData.Proj = DirectX::XMMatrixPerspectiveFovLH(component.CurrentFOV, component.Aspect, component.NearZ, component.FarZ);
//
//		component.CameraData.ViewProj = DirectX::XMMatrixMultiply(component.CameraData.View, component.CameraData.Proj);
//	}
//
//	float4 GetPosition4f()const { return float4(component.r_Position.x, component.r_Position.y, component.r_Position.z, 0.0f); }
//	float3 GetPosition3f()const { return component.r_Position; }
//	float4 GetRight4f()const { return float4(component.r_Right.x, component.r_Right.y, component.r_Right.z, 0.0f); }
//	float3 GetRight3f()const { return component.r_Right; }
//	float4 GetUp4f()const { return float4(component.r_Up.x, component.r_Up.y, component.r_Up.z, 0.0f); }
//	float3 GetUp3f()const { return component.r_Up; }
//	float4 GetLook4f()const { return float4(component.r_Look.x, component.r_Look.y, component.r_Look.z, 0.0f); }
//	float3 GetLook3f()const { return component.r_Look; }
//	float4 GetQuaternion() const {
//		DirectX::SimpleMath::Quaternion q;
//
//		double trace = component.r_Right.x + component.r_Up.y + component.r_Look.z;
//		if (trace > 0.0) {
//			double s = 0.5 / sqrt(trace + 1.0);
//			q.w = 0.25 / s;
//			q.x = (component.r_Up.z - component.r_Look.y) * s;
//			q.y = (component.r_Look.x - component.r_Right.z) * s;
//			q.z = (component.r_Right.y - component.r_Up.x) * s;
//		}
//		else {
//			if (component.r_Right.x > component.r_Up.y && component.r_Right.x > component.r_Look.z) {
//				double s = 2.0 * sqrt(1.0 + component.r_Right.x - component.r_Up.y - component.r_Look.z);
//				q.w = (component.r_Up.z - component.r_Look.y) / s;
//				q.x = 0.25 * s;
//				q.y = (component.r_Up.x + component.r_Right.y) / s;
//				q.z = (component.r_Look.x + component.r_Right.z) / s;
//			}
//			else if (component.r_Up.y > component.r_Look.z) {
//				double s = 2.0 * sqrt(1.0 + component.r_Up.y - component.r_Right.x - component.r_Look.z);
//				q.w = (component.r_Look.x - component.r_Right.z) / s;
//				q.x = (component.r_Up.x + component.r_Right.y) / s;
//				q.y = 0.25 * s;
//				q.z = (component.r_Look.y + component.r_Up.z) / s;
//			}
//			else {
//				double s = 2.0 * sqrt(1.0 + component.r_Look.z - component.r_Right.x - component.r_Up.y);
//				q.w = (component.r_Right.y - component.r_Up.x) / s;
//				q.x = (component.r_Look.x + component.r_Right.z) / s;
//				q.y = (component.r_Look.y + component.r_Up.z) / s;
//				q.z = 0.25 * s;
//			}
//		}
//		return q;
//	}
//	float GetNearZ()const { return component.NearZ; }
//	float GetFarZ()const { return component.FarZ; }
//	float GetAspect()const { return component.Aspect; }
//	float GetFovY()const { return component.CurrentFOV; }
//	float GetFovX()const {
//		float halfWidth = 0.5f * GetNearWindowWidth();
//		return 2.0f * atan(halfWidth / component.NearZ);
//	}
//	float GetNearWindowWidth()const { return component.Aspect * component.NearWindowHeight; }
//	float GetNearWindowHeight()const { return component.NearWindowHeight; }
//	float GetFarWindowWidth()const { return component.Aspect * component.FarWindowHeight; }
//	float GetFarWindowHeight()const { return component.FarWindowHeight; }
//	CameraData GetCameraData()const { return component.CameraData; }
//
//	// Set frustum.
//	void SetPosition(float x, float y, float z) {
//		component.Dirty = true;
//		component.w_Position = { x, y, z };
//	}
//	void SetPosition(const float3& v) {
//		component.Dirty = true;
//		component.w_Position = v;
//	}
//	void SetMovementSpeed(float speed) { component.MovementSpeed = speed; }
//	void SetRotationSpeed(float speed) { component.RotationSpeed = speed; }
//	void SetLens(float fovY, float aspect, float nearZ, float farZ)
//	{
//		component.Dirty = true;
//		component.CurrentFOV = fovY;
//		component.Aspect = aspect;
//		component.NearZ = nearZ;
//		component.FarZ = farZ;
//		component.NearWindowHeight = 2.0f * nearZ * tanf(0.5f * fovY);
//		component.FarWindowHeight = 2.0f * farZ * tanf(0.5f * fovY);
//	}
//	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp)
//	{
//		component.Dirty = true;
//		DirectX::XMVECTOR L = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(target, pos));
//		DirectX::XMVECTOR R = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(worldUp, L));
//		DirectX::XMVECTOR U = DirectX::XMVector3Cross(L, R);
//
//		DirectX::XMStoreFloat3(&component.w_Position, pos);
//		DirectX::XMStoreFloat3(&component.w_Look, L);
//		DirectX::XMStoreFloat3(&component.w_Right, R);
//		DirectX::XMStoreFloat3(&component.w_Up, U);
//	}
//	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up)
//	{
//		DirectX::XMVECTOR P = DirectX::XMLoadFloat3(&pos);
//		DirectX::XMVECTOR T = DirectX::XMLoadFloat3(&target);
//		DirectX::XMVECTOR U = DirectX::XMLoadFloat3(&up);
//
//		LookAt(P, T, U);
//	}
//	void Strafe(float d) { component.MovementDelta.x += d; }
//	void Walk(float d) { component.MovementDelta.z += d; }
//	void Up(float d) { component.MovementDelta.y += d; }
//	void Pitch(float angle) { component.PitchDelta += angle; }
//	void RotateY(float angle) { component.YawDelta += angle; }
//private:
//	CameraComponent component;
//};