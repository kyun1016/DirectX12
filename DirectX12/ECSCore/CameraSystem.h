#pragma once
#include "ECSCoordinator.h"
#include "CameraComponent.h"

class CameraSystem : public ECS::ISystem
{
public:
	void Update() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		for (ECS::Entity entity : mEntities) {
			auto& transform = coordinator.GetComponent<DX12_TransformComponent>(entity);
			transform.w_Position += {0.0f, 0.0f, 0.1f};
			transform.w_Scale += {0.0f, 0.0f, 0.1f};
			transform.w_RotationEuler += {0.0f, 0.0f, 0.1f};
			LOG_VERBOSE("transform {}, {}, {}", transform.w_Position.x, transform.w_Position.y, transform.w_Position.z);
		}
	}

	void FinalUpdate() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		for (ECS::Entity entity : mEntities) {
			auto& transform = coordinator.GetComponent<DX12_TransformComponent>(entity);
			if (transform.w_Position != transform.r_Position ||
				transform.w_Scale != transform.r_Scale ||
				transform.w_RotationEuler != transform.r_RotationEuler)
			{
				transform.Dirty = true;
				transform.r_Position = transform.w_Position;
				transform.r_Scale = transform.w_Scale;
				transform.r_RotationEuler = transform.w_RotationEuler;
			}
			else
			{
				transform.Dirty = false;
			}
		}
	}

	float4 GetPosition4f()const { return float4(component.r_Position.x, component.r_Position.y, component.r_Position.z, 0.0f); }
	float3 GetPosition3f()const	{ return component.r_Position; }
	float4 GetRight4f()const { return float4(component.r_Right.x, component.r_Right.y, component.r_Right.z, 0.0f); }
	float3 GetRight3f()const { return component.r_Right; }
	float4 GetUp4f()const { return float4(component.r_Up.x, component.r_Up.y, component.r_Up.z, 0.0f); }
	float3 GetUp3f()const { return component.r_Up; }
	float4 GetLook4f()const { return float4(component.r_Look.x, component.r_Look.y, component.r_Look.z, 0.0f); }
	float3 GetLook3f()const { return component.r_Look; }
	float4 GetQuaternion() const { 
		DirectX::SimpleMath::Quaternion q;

		double trace = component.r_Right.x + component.r_Up.y + component.r_Look.z;
		if (trace > 0.0) {
			double s = 0.5 / sqrt(trace + 1.0);
			q.w = 0.25 / s;
			q.x = (mUp.z - mLook.y) * s;
			q.y = (mLook.x - mRight.z) * s;
			q.z = (mRight.y - mUp.x) * s;
		}
		else {
			if (mRight.x > mUp.y && mRight.x > mLook.z) {
				double s = 2.0 * sqrt(1.0 + mRight.x - mUp.y - mLook.z);
				q.w = (mUp.z - mLook.y) / s;
				q.x = 0.25 * s;
				q.y = (mUp.x + mRight.y) / s;
				q.z = (mLook.x + mRight.z) / s;
			}
			else if (mUp.y > mLook.z) {
				double s = 2.0 * sqrt(1.0 + mUp.y - mRight.x - mLook.z);
				q.w = (mLook.x - mRight.z) / s;
				q.x = (mUp.x + mRight.y) / s;
				q.y = 0.25 * s;
				q.z = (mLook.y + mUp.z) / s;
			}
			else {
				double s = 2.0 * sqrt(1.0 + mLook.z - mRight.x - mUp.y);
				q.w = (mRight.y - mUp.x) / s;
				q.x = (mLook.x + mRight.z) / s;
				q.y = (mLook.y + mUp.z) / s;
				q.z = 0.25 * s;
			}
		}
		return q;
	}
	float GetNearZ()const { return mNearZ; }
	float GetFarZ()const { return mFarZ; }
	float GetAspect()const { return mAspect; }
	float GetFovY()const;
	float GetFovX()const;
	float GetNearWindowWidth()const;
	float GetNearWindowHeight()const;
	float GetFarWindowWidth()const;
	float GetFarWindowHeight()const;
	CameraData GetCameraData()const;

	// Set frustum.
	void SetPosition(float x, float y, float z);
	void SetPosition(const float3& v);
	void SetLens(float fovY, float aspect, float zn, float zf);
	void LookAt(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR worldUp);
	void LookAt(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& up);
	void Strafe(float d);
	void Walk(float d);
	void Up(float d);
	void Pitch(float angle);
	void RotateY(float angle);
	void UpdateViewMatrix();

private:
	CameraComponent component;
};