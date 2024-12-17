#include "AppSimple.h"
#include "AppBaseHelper.h"
#include "D3DUtil.h"

namespace kyun
{
	AppSimple::AppSimple() : AppSimple(1080, 720, L"AppSimple")	{	}
	AppSimple::AppSimple(uint32_t width, uint32_t height, std::wstring name)
		: AppBase(width, height, name)
		, m_rtvDescriptorSize(0)
		, m_dsvDescriptorSize(0)
		, m_constantBufferData{}
		, m_cbvDataBegin(nullptr)
		, m_frameIndex(0)
		, m_frameCounter(0)
		, m_fenceEvent{}
	{

	}
	bool AppSimple::OnInit()
	{
		if (!AppBase::OnInit())
			return false;

		// Reset the command list to prep for initialization commands.
		ThrowIfFailed(m_commandList->Reset(m_commandAllocator.Get(), nullptr));

		BuildDescriptorHeaps();
		BuildConstantBuffers();
		BuildRootSignature();
		BuildShadersAndInputLayout();
		BuildBoxGeometry();
		BuildPSO();

		// Execute the initialization commands.
		ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* cmdsLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		// Wait until initialization is complete.
		FlushCommandQueue();

		return true;
	}

	void AppSimple::OnResize()
	{
		AppBase::OnResize();

		XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, m_aspectRatio, 1.0f, 1000.0f);
		XMStoreFloat4x4(&m_proj, P);
	}

	void AppSimple::BuildDescriptorHeaps()
	{
		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc;
		cbvHeapDesc.NumDescriptors = 1;
		cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		cbvHeapDesc.NodeMask = 0;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_cbvHeap)));
	}
	void AppSimple::BuildConstantBuffers()
	{
		m_objectCB = std::make_unique<UploadBuffer<ObjectConstants>>(m_device.Get(), 1, true);

		UINT objCBByteSize = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_objectCB->Resource()->GetGPUVirtualAddress();
		// Offset to the ith object constant buffer in the buffer.
		int boxCBufIndex = 0;
		cbAddress += boxCBufIndex * objCBByteSize;

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = D3DUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

		m_device->CreateConstantBufferView(
			&cbvDesc,
			m_cbvHeap->GetCPUDescriptorHandleForHeapStart());
	}
	void AppSimple::BuildRootSignature()
	{
		// Shader programs typically require resources as input (constant buffers,
		// textures, samplers).  The root signature defines the resources the shader
		// programs expect.  If we think of the shader programs as a function, and
		// the input resources as function parameters, then the root signature can be
		// thought of as defining the function signature.  

		// Root parameter can be a table, root descriptor or root constants.
		CD3DX12_ROOT_PARAMETER slotRootParameter[1];

		// Create a single descriptor table of CBVs.
		CD3DX12_DESCRIPTOR_RANGE cbvTable;
		cbvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
		slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

		// A root signature is an array of root parameters.
		CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(1, slotRootParameter, 0, nullptr,
			D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
		ComPtr<ID3DBlob> serializedRootSig = nullptr;
		ComPtr<ID3DBlob> errorBlob = nullptr;
		HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
			serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

		if (errorBlob != nullptr)
		{
			::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
		}
		ThrowIfFailed(hr);

		ThrowIfFailed(m_device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(&m_rootSignature)));
	}
	}
	void AppSimple::BuildShadersAndInputLayout()
	{
	}
	void AppSimple::BuildBoxGeometry()
	{
	}
	void AppSimple::BuildPSO()
	{
	}
}