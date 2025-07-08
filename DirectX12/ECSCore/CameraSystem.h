#pragma once
#include "ECSCoordinator.h"
#include "CameraComponent.h"
#include "InputSystem.h"

class CameraSystem {
	DEFAULT_SINGLETON(CameraSystem)
public:
	void Initialize() {
		NewCamera();
	}
	void Sync() {
		auto currCameraBuffer = DX12_FrameResourceSystem::GetInstance().GetCurrentFrameResource().CameraDataBuffer.get();

		auto& coordinator = ECS::Coordinator::GetInstance();
		for (auto& camera: mAllCameras) {
			camera->SyncData();
			if (camera->NumFramesDirty)
			{
				camera->NumFramesDirty--;
				currCameraBuffer->CopyData(0, camera->CameraData); // Copy camera data to the current frame resource
			}
		}
	}
	size_t NewCamera() {
		mAllCameras.emplace_back(std::make_unique<CameraComponent>());

		return mAllCameras.size() - 1;
	}
	CameraComponent* GetCamera(size_t handle) {
		if (handle < mAllCameras.size()) {
			return mAllCameras[handle].get();
		}
		return nullptr;
	}
private:
	std::vector<std::unique_ptr<CameraComponent>> mAllCameras;
};