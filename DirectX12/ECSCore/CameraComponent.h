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

	float FieldOfView = XM_PIDIV4;
	float3 w_Position = { 0.0f, 0.0f, 0.0f };
	float3 w_Right = { 1.0f, 0.0f, 0.0f };
	float3 w_Up = { 0.0f, 1.0f, 0.0f };
	float3 w_Look = { 0.0f, 0.0f, 1.0f };
	float3 r_Position = { 0.0f, 0.0f, 0.0f };
	float3 r_Right = { 1.0f, 0.0f, 0.0f };
	float3 r_Up = { 0.0f, 1.0f, 0.0f };
	float3 r_Look = { 0.0f, 0.0f, 1.0f };
	float w_NearZ = 0.0f;
	float w_FarZ = 0.0f;
	float w_Aspect = 0.0f;
	float w_FovY = 0.0f;
	float w_NearWindowHeight = 0.0f;
	float w_FarWindowHeight = 0.0f;
	float w_Speed = 3.0f;
	float r_NearZ = 0.0f;
	float r_FarZ = 0.0f;
	float r_Aspect = 0.0f;
	float r_FovY = 0.0f;
	float r_NearWindowHeight = 0.0f;
	float r_FarWindowHeight = 0.0f;
	float r_Speed = 3.0f;
	bool Dirty = true;
	
};