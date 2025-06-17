#ifdef __cplusplus
#pragma once
#include "DX12_Config.h"
#endif

#ifndef MATERIAL_DATA_H
#define MATERIAL_DATA_H

struct MaterialData
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Roughness;
    float4x4 MatTransform;

    float Matalic;
    int DiffMapIndex;
    int NormMapIndex;
    int AOMapIndex;

    int MetallicMapIndex;
    int RoughnessMapIndex;
    int EmissiveMapIndex;
    int useAlbedoMap;

    int useNormalMap;
    int invertNormalMap;
    int useAOMap;
    int useMetallicMap;

    int useRoughnessMap;
    int useEmissiveMap;
    int useAlphaTest;
    int dummy1;
};

#endif // MATERIAL_DATA_H