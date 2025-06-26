#pragma once
#include "ECSCoordinator.h"
#include "DX12_InstanceComponent.h"

class DX12_InstanceSystem : public ECS::ISystem {
public:
    inline static DX12_InstanceSystem& GetInstance() {
        static DX12_InstanceSystem instance;
        return instance;
    }
    virtual ~DX12_InstanceSystem() = default; 

public:
    void Initialize() {
		// Assuming a max number of instances for simplicity.
		// In a real engine, this would be dynamic or based on scene data.
		const UINT maxInstances = 1000;
		mInstanceBuffer = std::make_unique<UploadBuffer<InstanceData>>(DX12_DeviceSystem::GetInstance().GetDevice(), maxInstances, false); // <- 해당 로직은 FrameSystem으로 이관
	}

    virtual void Update() override {
        // 추후에 컬링 시 오브젝트 판단 시 해당 시스템 사용을 고려해보자.
		// // Collect and update instance data
		// std::vector<InstanceData> instancesToUpload;
		// UINT instanceCount = 0;
		// for (auto const& entity : mEntities) {
		// 	auto& transform = ECS::Coordinator::GetInstance().GetComponent<DX12_TransformComponent>(entity);
		// 	InstanceData data;
			
		// 	DirectX::SimpleMath::Matrix worldMatrix = DirectX::SimpleMath::Matrix::CreateTranslation(transform.r_Position);
		// 	data.World = worldMatrix.Transpose();
		// 	instancesToUpload.push_back(data);
		// 	instanceCount++;
		// }

		// // Copy data to the upload buffer
		// if (!instancesToUpload.empty()) {
		// 	mInstanceBuffer->CopyData(0, instancesToUpload.data(), instancesToUpload.size());
		// }
	}

    D3D12_GPU_VIRTUAL_ADDRESS GetInstanceDataGPUVirtualAddress() const {
		return mInstanceBuffer->Resource()->GetGPUVirtualAddress();
	}
private:
    std::unique_ptr<UploadBuffer<InstanceData>> mInstanceBuffer;
private:
	DX12_InstanceSystem() = default; 
	DX12_InstanceSystem(const DX12_InstanceSystem&) = delete;
	DX12_InstanceSystem& operator=(const DX12_InstanceSystem&) = delete;
	DX12_InstanceSystem(DX12_InstanceSystem&&) = delete;
	DX12_InstanceSystem& operator=(DX12_InstanceSystem&&) = delete;
};