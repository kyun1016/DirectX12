//***************************************************************************************
// BlurFilter.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************
#include "pch.h"
#include "BlurFilter.h"

BlurFilter::BlurFilter(ID3D12Device* device,
	UINT width, UINT height,
	DXGI_FORMAT format)
{
	md3dDevice = device;

	mWidth = width;
	mHeight = height;
	mFormat = format;

	BuildShader();
	BuildResources();
	BuildRootSignature();
	BuildPSO();
}

UINT BlurFilter::DescriptorCount() const
{
	return 4;
}

ID3D12Resource* BlurFilter::Output()
{
	return mBlurMap0.Get();
}

void BlurFilter::BuildShader()
{
	mHorShader = D3DUtil::CompileShader(L"BlurCS.hlsl", nullptr, "HorBlurCS", "cs_5_0");
	mVerShader = D3DUtil::CompileShader(L"BlurCS.hlsl", nullptr, "VerBlurCS", "cs_5_0");
}

void BlurFilter::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor,
	UINT descriptorSize)
{
	// Save references to the descriptors. 
	mBlur0CpuSrv = hCpuDescriptor;
	mBlur0CpuUav = hCpuDescriptor.Offset(1, descriptorSize);
	mBlur1CpuSrv = hCpuDescriptor.Offset(1, descriptorSize);
	mBlur1CpuUav = hCpuDescriptor.Offset(1, descriptorSize);

	mBlur0GpuSrv = hGpuDescriptor;
	mBlur0GpuUav = hGpuDescriptor.Offset(1, descriptorSize);
	mBlur1GpuSrv = hGpuDescriptor.Offset(1, descriptorSize);
	mBlur1GpuUav = hGpuDescriptor.Offset(1, descriptorSize);

	BuildDescriptors();
}

void BlurFilter::BuildRootSignature()
{
	CD3DX12_DESCRIPTOR_RANGE srvTable;
	srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0

	CD3DX12_DESCRIPTOR_RANGE uavTable;
	uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0); // u0

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsConstants(12, 0);
	slotRootParameter[1].InitAsDescriptorTable(1, &srvTable);
	slotRootParameter[2].InitAsDescriptorTable(1, &uavTable);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(), IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void BlurFilter::BuildPSO()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC BlurHorPSO = {
		/* ID3D12RootSignature * pRootSignature		*/.pRootSignature = mRootSignature.Get(),
		/* D3D12_SHADER_BYTECODE CS					*/.CS = { reinterpret_cast<BYTE*>(mHorShader->GetBufferPointer()), mHorShader->GetBufferSize() },
		/* UINT NodeMask							*/.NodeMask = 0,
		/* D3D12_CACHED_PIPELINE_STATE CachedPSO	*/.CachedPSO = 0,
		/* D3D12_PIPELINE_STATE_FLAGS Flags			*/.Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
	};

	// PSO for vertical blur
	D3D12_COMPUTE_PIPELINE_STATE_DESC BlurVerPSO = BlurHorPSO;
	BlurVerPSO.CS = { reinterpret_cast<BYTE*>(mVerShader->GetBufferPointer()), mVerShader->GetBufferSize() };

	ThrowIfFailed(md3dDevice->CreateComputePipelineState(&BlurHorPSO, IID_PPV_ARGS(&mHorPSO)));
	ThrowIfFailed(md3dDevice->CreateComputePipelineState(&BlurVerPSO, IID_PPV_ARGS(&mVerPSO)));
}

void BlurFilter::OnResize(UINT newWidth, UINT newHeight)
{
	mWidth = newWidth;
	mHeight = newHeight;

	BuildResources();

	// New resource, so we need new descriptors to that resource.
	BuildDescriptors();
}

void BlurFilter::Execute(ID3D12GraphicsCommandList* cmdList,
	ID3D12Resource* input,
	int blurCount)
{
	auto weights = CalcGaussWeights(2.5f);
	int blurRadius = (int)weights.size() / 2;

	cmdList->SetComputeRootSignature(mRootSignature.Get());

	cmdList->SetComputeRoot32BitConstants(0, 1, &blurRadius, 0);
	cmdList->SetComputeRoot32BitConstants(0, (UINT)weights.size(), weights.data(), 1);

	D3D12_RESOURCE_BARRIER barrierInput = CD3DX12_RESOURCE_BARRIER::Transition(input, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
	D3D12_RESOURCE_BARRIER barrierMap0 = CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap0.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
	D3D12_RESOURCE_BARRIER barrierMap1 = CD3DX12_RESOURCE_BARRIER::Transition(mBlurMap1.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	cmdList->ResourceBarrier(1, &barrierInput);
	cmdList->ResourceBarrier(1, &barrierMap0);
	// Copy the input (back-buffer in this example) to BlurMap0.
	cmdList->CopyResource(mBlurMap0.Get(), input);

	barrierMap0.Transition.StateBefore = barrierMap0.Transition.StateAfter;
	barrierMap0.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	cmdList->ResourceBarrier(1, &barrierMap0);
	cmdList->ResourceBarrier(1, &barrierMap1);

	for (int i = 0; i < blurCount; ++i)
	{
		//
		// Horizontal Blur pass.
		//

		cmdList->SetPipelineState(mHorPSO.Get());

		cmdList->SetComputeRootDescriptorTable(1, mBlur0GpuSrv);
		cmdList->SetComputeRootDescriptorTable(2, mBlur1GpuUav);


		// How many groups do we need to dispatch to cover a row of pixels, where each
		// group covers 256 pixels (the 256 is defined in the ComputeShader).
		UINT numGroupsX = (UINT)ceilf(mWidth / 256.0f);
		cmdList->Dispatch(numGroupsX, mHeight, 1);
		barrierMap0.Transition.StateBefore = barrierMap0.Transition.StateAfter;
		barrierMap0.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		barrierMap1.Transition.StateBefore = barrierMap1.Transition.StateAfter;
		barrierMap1.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		cmdList->ResourceBarrier(1, &barrierMap0);
		cmdList->ResourceBarrier(1, &barrierMap1);


		//
		// Vertical Blur pass.
		//

		cmdList->SetPipelineState(mVerPSO.Get());

		cmdList->SetComputeRootDescriptorTable(1, mBlur1GpuSrv);
		cmdList->SetComputeRootDescriptorTable(2, mBlur0GpuUav);



		// How many groups do we need to dispatch to cover a column of pixels, where each
		// group covers 256 pixels  (the 256 is defined in the ComputeShader).
		UINT numGroupsY = (UINT)ceilf(mHeight / 256.0f);
		cmdList->Dispatch(mWidth, numGroupsY, 1);

		barrierMap0.Transition.StateBefore = barrierMap0.Transition.StateAfter;
		barrierMap0.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
		barrierMap1.Transition.StateBefore = barrierMap1.Transition.StateAfter;
		barrierMap1.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
		cmdList->ResourceBarrier(1, &barrierMap0);
		cmdList->ResourceBarrier(1, &barrierMap1);
	}

	barrierInput.Transition.StateBefore = barrierInput.Transition.StateAfter;
	barrierInput.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierMap0.Transition.StateBefore = barrierMap0.Transition.StateAfter;
	barrierMap0.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_SOURCE;
	barrierMap1.Transition.StateBefore = barrierMap1.Transition.StateAfter;
	barrierMap1.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;
	cmdList->ResourceBarrier(1, &barrierInput);
	cmdList->ResourceBarrier(1, &barrierMap0);
	cmdList->ResourceBarrier(1, &barrierMap1);

	D3D12_RESOURCE_BARRIER RenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(input, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
	cmdList->ResourceBarrier(1, &RenderBarrier);

	cmdList->CopyResource(input, mBlurMap0.Get());

	RenderBarrier = CD3DX12_RESOURCE_BARRIER::Transition(input, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_RENDER_TARGET);
	cmdList->ResourceBarrier(1, &RenderBarrier);
}

std::vector<float> BlurFilter::CalcGaussWeights(float sigma)
{
	float twoSigma2 = 2.0f * sigma * sigma;

	// Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
	// For example, for sigma = 3, the width of the bell curve is 
	int blurRadius = (int)ceil(2.0f * sigma);

	assert(blurRadius <= MaxBlurRadius);

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);

	float weightSum = 0.0f;

	for (int i = -blurRadius; i <= blurRadius; ++i)
	{
		float x = (float)i;

		weights[i + blurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + blurRadius];
	}

	// Divide by the sum so all the weights add up to 1.0.
	for (int i = 0; i < weights.size(); ++i)
	{
		weights[i] /= weightSum;
	}

	return weights;
}

void BlurFilter::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = mFormat;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

	uavDesc.Format = mFormat;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	md3dDevice->CreateShaderResourceView(mBlurMap0.Get(), &srvDesc, mBlur0CpuSrv);
	md3dDevice->CreateUnorderedAccessView(mBlurMap0.Get(), nullptr, &uavDesc, mBlur0CpuUav);

	md3dDevice->CreateShaderResourceView(mBlurMap1.Get(), &srvDesc, mBlur1CpuSrv);
	md3dDevice->CreateUnorderedAccessView(mBlurMap1.Get(), nullptr, &uavDesc, mBlur1CpuUav);
}

void BlurFilter::BuildResources()
{
	// Note, compressed formats cannot be used for UAV.  We get error like:
	// ERROR: ID3D11Device::CreateTexture2D: The format (0x4d, BC3_UNORM) 
	// cannot be bound as an UnorderedAccessView, or cast to a format that
	// could be bound as an UnorderedAccessView.  Therefore this format 
	// does not support D3D11_BIND_UNORDERED_ACCESS.

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = mWidth;
	texDesc.Height = mHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = mFormat;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_HEAP_PROPERTIES heapProperty
	{
		/* D3D12_HEAP_TYPE Type						*/.Type = D3D12_HEAP_TYPE_DEFAULT,
		/* D3D12_CPU_PAGE_PROPERTY CPUPageProperty	*/.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		/* D3D12_MEMORY_POOL MemoryPoolPreference	*/.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		/* UINT CreationNodeMask					*/.CreationNodeMask = 1,
		/* UINT VisibleNodeMask						*/.VisibleNodeMask = 1,
	};
	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mBlurMap0)));

	ThrowIfFailed(md3dDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&mBlurMap1)));
}