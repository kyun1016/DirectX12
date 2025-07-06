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

	bool Dirty = true;
	float DefaultFOV = DirectX::XM_PIDIV4;
	float FieldOfView = XM_PIDIV4;
	float MovementSpeed = 3.0f;
	float RotationSpeed = 0.5f;
	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float Aspect = 0.0f;
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
};