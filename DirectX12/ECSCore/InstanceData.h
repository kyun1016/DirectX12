#ifdef __cplusplus
#pragma once
#include "ECSConfig.h"
#endif

#ifndef INSTANCE_DATA_H
#define INSTANCE_DATA_H

struct InstanceData
{
#ifdef __cplusplus
	static const char* GetName() { return "InstanceData"; }
#endif
    float4x4 World;
    float4x4 TexTransform;
    float4x4 WorldInvTranspose;

    float2 DisplacementMapTexelSize;
    float GridSpatialStep;
    uint useDisplacementMap;

    uint DisplacementIndex;
    uint MaterialIndex;
    uint CameraIndex;
    uint LightIndex;

    // TODO: 그 외 다양한 속성을 Structured Buffer에서 관리하는 경우, Index 성분 추가
};
struct InstanceIDData
{
    uint BaseInstanceIndex;
};
#ifdef __cplusplus
// 해당 이유는 InstanceData 구조체는 HLSL에서 공용으로 쓰기 위해 선언하였기 때문으로
// C++의 JSON 변환만을 위한 별도의 함수 정의
inline void to_json(json& j, const InstanceData& p) {
	// World, TexTransform, WorldInvTranspose는 DirectX::XMMATRIX로 변환되어야 하므로 생략 가능
	j = json{
		{ "DisplacementMapTexelSize", { p.DisplacementMapTexelSize.x, p.DisplacementMapTexelSize.y } },
		{ "GridSpatialStep",			p.GridSpatialStep },
		{ "useDisplacementMap",			p.useDisplacementMap },
		{ "DisplacementIndex",			p.DisplacementIndex },
		{ "MaterialIndex",				p.MaterialIndex },
		{ "CameraIndex",				p.CameraIndex },
		{ "LightIndex",					p.LightIndex }
	};
}

inline void from_json(const json& j, InstanceData& p) {
	p.DisplacementMapTexelSize.x = j.at("DisplacementMapTexelSize")[0].get<float>();
	p.DisplacementMapTexelSize.y = j.at("DisplacementMapTexelSize")[1].get<float>();
	p.GridSpatialStep = j.at("GridSpatialStep").get<float>();
	p.useDisplacementMap = j.at("useDisplacementMap").get<uint>();
	p.DisplacementIndex = j.at("DisplacementIndex").get<uint>();
	p.MaterialIndex = j.at("MaterialIndex").get<uint>();
	p.CameraIndex = j.at("CameraIndex").get<uint>();
	p.LightIndex = j.at("LightIndex").get<uint>();
}
#endif
#endif // INSTANCE_DATA_H