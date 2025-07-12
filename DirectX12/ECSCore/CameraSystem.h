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
		auto& input = InputSystem::GetInstance();
		float3 moveDir = { 0.0f, 0.0f, 0.0f };

		if (input.IsKeyDown('A')) moveDir.x -= 1.0f;
		if (input.IsKeyDown('a')) moveDir.x -= 1.0f;
		if (input.IsKeyDown('D')) moveDir.x += 1.0f;
		if (input.IsKeyDown('d')) moveDir.x += 1.0f;
		if (input.IsKeyDown('Q')) moveDir.y -= 1.0f;
		if (input.IsKeyDown('q')) moveDir.y -= 1.0f;
		if (input.IsKeyDown('E')) moveDir.y += 1.0f;
		if (input.IsKeyDown('e')) moveDir.y += 1.0f;
		if (input.IsKeyDown('W')) moveDir.z += 1.0f;
		if (input.IsKeyDown('w')) moveDir.z += 1.0f;
		if (input.IsKeyDown('S')) moveDir.z -= 1.0f;
		if (input.IsKeyDown('s')) moveDir.z -= 1.0f;
		moveDir *= ECS::Coordinator::GetInstance().GetSingletonComponent<TimeComponent>().deltaTime;
		LOG_INFO("delta: {} || called with moveDir: {}, {}, {}", ECS::Coordinator::GetInstance().GetSingletonComponent<TimeComponent>().deltaTime, moveDir.x, moveDir.y, moveDir.z);
		

		auto currCameraBuffer = DX12_FrameResourceSystem::GetInstance().GetCurrentFrameResource().CameraDataBuffer.get();
		for (auto& camera: mAllCameras) {
			camera->Strafe(moveDir.x);
			camera->Up(moveDir.y);
			camera->Walk(moveDir.z);
			camera->SyncData();
			LOG_INFO("Camera Position: {}, {}, {}", camera->r_Position.x, camera->r_Position.y, camera->r_Position.z);
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