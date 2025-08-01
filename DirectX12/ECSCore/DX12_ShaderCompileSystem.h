#pragma once
#include "DX12_Config.h"
#include "ECSSystem.h"
#include <d3dcompiler.h>

class DX12_ShaderCompileSystem
{
DEFAULT_SINGLETON(DX12_ShaderCompileSystem)
public:
	inline void Initialize(size_t textureSize = 1) 
	{
		char texDiffSize[100];
		sprintf_s(texDiffSize, "%d", static_cast<int>(textureSize));
		const D3D_SHADER_MACRO defines[] =
		{
			"TEXTURE_DIFFUSE_SIZE", texDiffSize,
			NULL, NULL
		};

		CompileShader("vs_main", L"../Data/Shaders/Main.hlsl", nullptr, "VS", "vs_5_1");
		CompileShader("ps_main", L"../Data/Shaders/Main.hlsl", nullptr, "PS", "ps_5_1");

		CompileShader("vs_test", L"../Data/Shaders/test.hlsl", nullptr, "VS", "vs_5_1");
		CompileShader("ps_test", L"../Data/Shaders/test.hlsl", nullptr, "PS", "ps_5_1");

		CompileShader("vs_sprite", L"../Data/Shaders/Sprite.hlsl", defines, "VS", "vs_5_1");
		CompileShader("gs_sprite", L"../Data/Shaders/Sprite.hlsl", defines, "GS", "gs_5_1");
		CompileShader("ps_sprite", L"../Data/Shaders/Sprite.hlsl", defines, "PS", "ps_5_1");
	}

	inline void CompileShader(
		const std::string& name,
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entryPoint,
		const std::string& target)
	{
		mShaderMap[name] = CompileShader(filename, defines, entryPoint, target);
	}

	inline ID3DBlob* GetShader(const std::string& name) const
	{
		auto it = mShaderMap.find(name);
		return (it != mShaderMap.end()) ? it->second.Get() : nullptr;
	}

private:
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaderMap;

	Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint, const std::string& target)
	{
		UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
		compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

		HRESULT hr = S_OK;

		Microsoft::WRL::ComPtr<ID3DBlob> byteCode = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> errors;
		hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
			entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

		if (errors != nullptr)
			LOG_ERROR("{}", (char*)errors->GetBufferPointer());

		ThrowIfFailed(hr);

		return byteCode;
	}
};