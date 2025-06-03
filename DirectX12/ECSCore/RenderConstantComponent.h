#pragma once
#include "../EngineCore/D3DUtil.h"
#include "RenderComponent.h"

struct SkinnedConstantComponent
{
	DirectX::SimpleMath::Matrix BoneTransforms[96];
};

struct ShaderToyConstantComponent
{
	DirectX::SimpleMath::Vector4 iMouse;

	float dx;
	float dy;
	float threshold;
	float strength;

	float iTime;
	DirectX::SimpleMath::Vector2 iResolution;
	float dummy;
};

struct SsaoConstantComponent
{
	DirectX::SimpleMath::Matrix Proj;
	DirectX::SimpleMath::Matrix InvProj;
	DirectX::SimpleMath::Matrix ProjTex;
	DirectX::SimpleMath::Vector4 OffsetVectors[14];

	// For SsaoBlur.hlsl
	DirectX::SimpleMath::Vector4 BlurWeights[3];

	DirectX::SimpleMath::Vector2 InvRenderTargetSize = { 0.0f, 0.0f };

	// Coordinates given in view space.
	float OcclusionRadius = 0.5f;
	float OcclusionFadeStart = 0.2f;
	float OcclusionFadeEnd = 2.0f;
	float SurfaceEpsilon = 0.05f;
};

struct PassConstantComponent
{
	DirectX::SimpleMath::Matrix View = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix InvView = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix Proj = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix InvProj = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix ViewProj = MathHelper::Identity4x4();
	DirectX::SimpleMath::Matrix InvViewProj = MathHelper::Identity4x4();

	DirectX::SimpleMath::Vector3 EyePosW = { 0.0f, 0.0f, 0.0f };
	float cbPerObjectPad1 = 0.0f;

	DirectX::SimpleMath::Vector2 RenderTargetSize = { 0.0f, 0.0f };
	DirectX::SimpleMath::Vector2 InvRenderTargetSize = { 0.0f, 0.0f };

	float NearZ = 0.0f;
	float FarZ = 0.0f;
	float TotalTime = 0.0f;
	float DeltaTime = 0.0f;

	DirectX::SimpleMath::Vector4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	DirectX::SimpleMath::Vector4 FogColor = { 0.7f, 0.7f, 0.7f, 1.0f };

	float gFogStart = 5.0f;
	float gFogRange = 150.0f;
	DirectX::SimpleMath::Vector2 cbPerObjectPad2 = { 0.0f, 0.0f };

	LightComponent Lights[MAX_LIGHTS];

	uint32_t gCubeMapIndex = 0;
};

struct InstanceConstantComponent
{
	uint32_t BaseInstanceIndex;
};