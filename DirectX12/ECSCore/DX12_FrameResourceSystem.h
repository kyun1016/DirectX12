#pragma once

#include "DX12_Config.h"
#include "DX12_InstanceData.h"
#include "DX12_PassData.h"

struct DX12_FrameResource {
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	UINT64 fenceValue = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	std::unique_ptr<UploadBuffer<InstanceData>> InstanceDataBuffer;
	std::unique_ptr<UploadBuffer<InstanceIDData>> InstanceIDCB;
	std::unique_ptr<UploadBuffer<PassData>> PassCB;
};

struct DX12_FrameResourceSystem
{
public:
	inline static DX12_FrameResourceSystem& GetInstance() {
		static DX12_FrameResourceSystem instance;
		return instance;
	}

	void Initialize(ID3D12Device* device, const std::uint32_t instanceCount = 1000)
	{
		for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; ++i)
		{
			ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(mFrameResources[i].commandAllocator.GetAddressOf())));
			mFrameResources[i].InstanceDataBuffer = std::make_unique<UploadBuffer<InstanceData>>(device, instanceCount, false);
		}
	}
	void BeginFrame()
	{
		DX12_CommandSystem::GetInstance().FlushCommandQueue(mFrameResources[mCurrFrameResourceIndex].fenceValue);
		mFrameResources[mCurrFrameResourceIndex].commandAllocator->Reset();
		DX12_CommandSystem::GetInstance().ResetCommandList(mFrameResources[mCurrFrameResourceIndex].commandAllocator.Get());

		{
			// if (BaseBoundingBox)
			// {
			// 	BoundingBox.Center.x = BaseBoundingBox->Center.x * Scale.x + Translation.x;
			// 	BoundingBox.Center.y = BaseBoundingBox->Center.y * Scale.y + Translation.y;
			// 	BoundingBox.Center.z = BaseBoundingBox->Center.z * Scale.z + Translation.z;

			// 	BoundingBox.Extents.x = BaseBoundingBox->Extents.x * Scale.x;
			// 	BoundingBox.Extents.y = BaseBoundingBox->Extents.y * Scale.y;
			// 	BoundingBox.Extents.z = BaseBoundingBox->Extents.z * Scale.z;
			// }
			// if (BaseBoundingSphere)
			// {
			// 	BoundingSphere.Center.x = BaseBoundingSphere->Center.x * Scale.x + Translation.x;
			// 	BoundingSphere.Center.y = BaseBoundingSphere->Center.y * Scale.y + Translation.y;
			// 	BoundingSphere.Center.z = BaseBoundingSphere->Center.z * Scale.z + Translation.z;
			// 	BoundingSphere.Radius = BaseBoundingSphere->Radius * Scale.Length();
			// }

			// float rx = DirectX::XMConvertToRadians(Rotate.x);
			// float ry = DirectX::XMConvertToRadians(Rotate.y);
			// float rz = DirectX::XMConvertToRadians(Rotate.z);
			// RotationQuat = DirectX::XMQuaternionRotationRollPitchYaw(rx, ry, rz);

			// DirectX::XMMATRIX rotX = DirectX::XMMatrixRotationX(rx);
			// DirectX::XMMATRIX rotY = DirectX::XMMatrixRotationY(ry);
			// DirectX::XMMATRIX rotZ = DirectX::XMMatrixRotationZ(rz);

			// DirectX::XMMATRIX rot = rotX * rotY * rotZ;
			// if (useQuat)
			// 	rot = DirectX::XMMatrixRotationQuaternion(RotationQuat);

			// DirectX::XMMATRIX world
			// 	= DirectX::XMMatrixScaling(Scale.x, Scale.y, Scale.z)
			// 	* rot
			// 	* DirectX::XMMatrixTranslation(Translation.x, Translation.y, Translation.z);
			// DirectX::XMMATRIX texTransform = DirectX::XMMatrixScaling(TexScale.x, TexScale.y, TexScale.z);
			// DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(world);

			// InstanceData.World = DirectX::XMMatrixTranspose(world);
			// InstanceData.TexTransform = DirectX::XMMatrixTranspose(texTransform);
			// InstanceData.WorldInvTranspose = DirectX::XMMatrixInverse(&det, world);

			// InstanceData instanceData;
			// instanceData.World = float4x4::Identity;
		}
		
		
		mFrameResources[mCurrFrameResourceIndex].InstanceDataBuffer->CopyData(0, InstanceData{});
	}
	void EndFrame()
	{
		mFrameResources[mCurrFrameResourceIndex].fenceValue = DX12_CommandSystem::GetInstance().SetSignalFence();
		mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % APP_NUM_BACK_BUFFERS;
	}

	// 내부에 Frame value 관리 자체를 제거함 (SwapChainSystem에서 통합 관리)
	inline DX12_FrameResource& GetFrameResource(std::uint64_t frameIndex) {
		return mFrameResources[frameIndex];
	}
	inline DX12_FrameResource& GetCurrentFrameResource() {
		return mFrameResources[mCurrFrameResourceIndex];
	}

	D3D12_GPU_VIRTUAL_ADDRESS GetInstanceDataGPUVirtualAddress() const {
		return mFrameResources[mCurrFrameResourceIndex].InstanceDataBuffer->Resource()->GetGPUVirtualAddress();
	}

private:
	ID3D12CommandQueue* mCommandQueue = nullptr;
	std::vector<DX12_FrameResource> mFrameResources;
	DX12_FrameResource* mCurrFrameResource = nullptr;
	std::uint64_t mCurrFrameResourceIndex = 0;

	DX12_FrameResourceSystem()
	{
		mFrameResources.resize(APP_NUM_BACK_BUFFERS);
		for (UINT i = 0; i < APP_NUM_BACK_BUFFERS; ++i)
		{
			mFrameResources[i].fenceValue = 0;
			mFrameResources[i].rtvHandle.ptr = 0;
		}
		LOG_INFO("DX12 Frame Resource System Initialized");
	};
	~DX12_FrameResourceSystem() = default;
	DX12_FrameResourceSystem(const DX12_FrameResourceSystem&) = delete;
	DX12_FrameResourceSystem& operator=(const DX12_FrameResourceSystem&) = delete;
	DX12_FrameResourceSystem(DX12_FrameResourceSystem&&) = delete;
	DX12_FrameResourceSystem& operator=(DX12_FrameResourceSystem&&) = delete;
};