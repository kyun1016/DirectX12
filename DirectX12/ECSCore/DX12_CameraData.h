#ifdef __cplusplus
#pragma once
#include "DX12_Config.h"
#endif

#ifndef PASS_DATA_H
#define PASS_DATA_H

struct CamereData
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float FieldOfView = XM_PIDIV4; // 45도 시야각

    float gNearZ;
    float gFarZ;
};
#endif // PASS_DATA_H