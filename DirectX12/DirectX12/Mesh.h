#pragma once

#include <wrl/client.h>

// #include "Texture3D.h"
//
//namespace kyun {
//
//    using Microsoft::WRL::ComPtr;
//
//    struct Mesh {
//        ComPtr<ID3D12Buffer> vertexBuffer;
//        ComPtr<ID3D11Buffer> indexBuffer;
//
//        ComPtr<ID3D11Buffer> meshConstsGPU;
//        ComPtr<ID3D11Buffer> materialConstsGPU;
//
//        ComPtr<ID3D11Texture2D> albedoTexture;
//        ComPtr<ID3D11Texture2D> emissiveTexture;
//        ComPtr<ID3D11Texture2D> normalTexture;
//        ComPtr<ID3D11Texture2D> heightTexture;
//        ComPtr<ID3D11Texture2D> aoTexture;
//        ComPtr<ID3D11Texture2D> metallicRoughnessTexture;
//
//        ComPtr<ID3D11ShaderResourceView> albedoSRV;
//        ComPtr<ID3D11ShaderResourceView> emissiveSRV;
//        ComPtr<ID3D11ShaderResourceView> normalSRV;
//        ComPtr<ID3D11ShaderResourceView> heightSRV;
//        ComPtr<ID3D11ShaderResourceView> aoSRV;
//        ComPtr<ID3D11ShaderResourceView> metallicRoughnessSRV;
//
//        //// 3D Textures
//        //Texture3D densityTex;
//        //Texture3D lightingTex;
//
//        UINT indexCount = 0;
//        UINT vertexCount = 0;
//        UINT stride = 0;
//        UINT offset = 0;
//    };
//
//}
