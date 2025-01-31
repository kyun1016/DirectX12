#include "pch.h"
#include "CSAdd.h"
#include "../EngineCore/D3DUtil.h"
#include "AppBase.h"

CSAdd::CSAdd(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
	: mDevice(device)
{
	BuildShader();
	BuildResources(cmdList);
	BuildRootSignature();
	BuildPSO();
}

void CSAdd::BuildShader()
{
	mShader = D3DUtil::LoadBinary(L"Shaders\\AddCS.cso");
}

void CSAdd::BuildResources(ID3D12GraphicsCommandList* cmdList)
{
	// Generate some data.
	std::vector<Data> dataA(NumDataElements);
	std::vector<Data> dataB(NumDataElements);
	for (int i = 0; i < NumDataElements; ++i)
	{
		dataA[i].v1 = DirectX::XMFLOAT3(i, i, i);
		dataA[i].v2 = DirectX::XMFLOAT2(i, 0);

		dataB[i].v1 = DirectX::XMFLOAT3(-i, i, 0.0f);
		dataB[i].v2 = DirectX::XMFLOAT2(0, -i);
	}

	UINT64 byteSize = dataA.size() * sizeof(Data);

	// Create some buffers to be used as SRVs.
	mInputBufferA = D3DUtil::CreateDefaultBuffer(mDevice, cmdList, dataA.data(), byteSize, mInputUploadBufferA);
	mInputBufferB = D3DUtil::CreateDefaultBuffer(mDevice, cmdList, dataB.data(), byteSize, mInputUploadBufferB);

	// Create the buffer that will be a UAV.
	D3D12_HEAP_PROPERTIES heapProperty
	{
		/* D3D12_HEAP_TYPE Type						*/.Type = D3D12_HEAP_TYPE_DEFAULT,
		/* D3D12_CPU_PAGE_PROPERTY CPUPageProperty	*/.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		/* D3D12_MEMORY_POOL MemoryPoolPreference	*/.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
		/* UINT CreationNodeMask					*/.CreationNodeMask = 1,
		/* UINT VisibleNodeMask						*/.VisibleNodeMask = 1,
	};

	D3D12_RESOURCE_DESC bufferDesc
	{
		/* D3D12_RESOURCE_DIMENSION Dimension	*/.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
		/* UINT64 Alignment						*/.Alignment = 0,
		/* UINT64 Width							*/.Width = byteSize,
		/* UINT Height							*/.Height = 1,
		/* UINT16 DepthOrArraySize				*/.DepthOrArraySize = 1,
		/* UINT16 MipLevels						*/.MipLevels = 1,
		/* DXGI_FORMAT Format					*/.Format = DXGI_FORMAT_UNKNOWN,
		/* DXGI_SAMPLE_DESC SampleDesc{			*/.SampleDesc = {
		/*	UINT Count							*/		.Count = 1,
		/*	UINT Quality}						*/		.Quality = 0},
		/* D3D12_TEXTURE_LAYOUT Layout			*/.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
		/* D3D12_RESOURCE_FLAGS Flags			*/.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
	};
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
		nullptr,
		IID_PPV_ARGS(&mOutputBuffer)));

	heapProperty.Type = D3D12_HEAP_TYPE_READBACK;
	bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	ThrowIfFailed(mDevice->CreateCommittedResource(
		&heapProperty,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&mReadBackBuffer)));
}

void CSAdd::BuildRootSignature()
{
	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[3];

	// Perfomance TIP: Order from most frequent to least frequent.
	slotRootParameter[0].InitAsShaderResourceView(0);
	slotRootParameter[1].InitAsShaderResourceView(1);
	slotRootParameter[2].InitAsUnorderedAccessView(0);

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
		0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_NONE);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serializedRootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	ThrowIfFailed(hr);

	ThrowIfFailed(mDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void CSAdd::BuildPSO()
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {
		/* ID3D12RootSignature * pRootSignature		*/.pRootSignature = mRootSignature.Get(),
		/* D3D12_SHADER_BYTECODE CS					*/.CS = { reinterpret_cast<BYTE*>(mShader->GetBufferPointer()), mShader->GetBufferSize() },
		/* UINT NodeMask							*/.NodeMask = 0,
		/* D3D12_CACHED_PIPELINE_STATE CachedPSO	*/.CachedPSO = 0,
		/* D3D12_PIPELINE_STATE_FLAGS Flags			*/.Flags = D3D12_PIPELINE_STATE_FLAG_NONE,
	};
	ThrowIfFailed(mDevice->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(mPSO.GetAddressOf())));
}

void CSAdd::DoComputeWork(ID3D12GraphicsCommandList* cmdList, ID3D12CommandAllocator* cmdAlloc)
{
	ThrowIfFailed(cmdAlloc->Reset());
	ThrowIfFailed(cmdList->Reset(cmdAlloc, nullptr));

	cmdList->SetComputeRootSignature(mRootSignature.Get());

	cmdList->SetComputeRootShaderResourceView(0, mInputBufferA->GetGPUVirtualAddress());
	cmdList->SetComputeRootShaderResourceView(1, mInputBufferB->GetGPUVirtualAddress());
	cmdList->SetComputeRootUnorderedAccessView(2, mOutputBuffer->GetGPUVirtualAddress());

	cmdList->Dispatch(1, 1, 1);

	// Schedule to copy the data to the default buffer to the readback buffer.
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_SOURCE);
	cmdList->ResourceBarrier(1, &barrier);

	cmdList->CopyResource(mReadBackBuffer.Get(), mOutputBuffer.Get());

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(mOutputBuffer.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
	cmdList->ResourceBarrier(1, &barrier);
}

void CSAdd::PrintOutput()
{
	//if (!AppBase::g_appBase)
	//	return;
	//AppBase::g_appBase->FlushCommandQueue();

	// Map the data so we can read it on CPU.
	Data* mappedData = nullptr;
	ThrowIfFailed(mReadBackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

	std::ofstream fout("results.txt");

	for (int i = 0; i < NumDataElements; ++i)
	{
		fout << "(" << mappedData[i].v1.x << ", " << mappedData[i].v1.y << ", " << mappedData[i].v1.z <<
			", " << mappedData[i].v2.x << ", " << mappedData[i].v2.y << ")" << std::endl;
	}

	mReadBackBuffer->Unmap(0, nullptr);
}