#pragma once
#include "CameraData.h"

struct CameraComponent
{
	float3 mPosition = { 0.0f, 0.0f, 0.0f };
	float3 mRight = { 1.0f, 0.0f, 0.0f };
	float3 mUp = { 0.0f, 1.0f, 0.0f };
	float3 mLook = { 0.0f, 0.0f, 1.0f };

	float mNearZ = 0.0f;
	float mFarZ = 0.0f;
	float mAspect = 0.0f;
	float mFovY = 0.0f;
	float mNearWindowHeight = 0.0f;
	float mFarWindowHeight = 0.0f;

	bool mViewDirty = true;

	float mSpeed = 3.0f;

	DirectX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();
};