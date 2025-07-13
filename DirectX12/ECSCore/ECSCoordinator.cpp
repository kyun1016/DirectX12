#pragma once
#include "pch.h"

#include "ECSEntity.h"
#include "ECSComponent.h"
#include "ECSSystem.h"
#include "WindowSystem.h"
#include "ImGuiSystem.h"

namespace ECS
{
	void Coordinator::Init()
	{
		// Create pointers to each manager
		mComponentManager = std::make_unique<ComponentManager>();
		mEntityManager = std::make_unique<EntityManager>();
		mSystemManager = std::make_unique<SystemManager>();
	}
	Entity Coordinator::CreateEntity()
	{
		return mEntityManager->CreateEntity();
	}
	void Coordinator::DestroyEntity(Entity entity)
	{
		std::lock_guard<std::mutex> lock(mtx);
		mEntityManager->DestroyEntity(entity);
		mComponentManager->EntityDestroyed(entity);
		mSystemManager->EntityDestroyed(entity);
	}
	void Coordinator::Run()
	{
		mSystemManager->BeginPlayAllSystems();
		while (true) // Replace with actual game loop condition
		{
			//#########################
			// WinProc의 경우 병렬처리를 통해 쓰레드를 가져가는 순간 오류 발생
			// 메인 쓰레드에서 처리가 이뤄지도록 예외적으로 핸들링을 진행
			//#########################
			if (!WindowSystem::GetInstance().Sync())
				break;
			InputSystem::GetInstance().PreUpdate();
			mSystemManager->SyncAllSystems();
			mSystemManager->PreUpdateAllSystems();
			mSystemManager->UpdateAllSystems();
			ImGuiSystem::GetInstance().RenderMultiViewport();
			mSystemManager->LateUpdateAllSystems();
			mSystemManager->FixedUpdateAllSystems();
			mSystemManager->FinalUpdateAllSystems();
		}
		mSystemManager->EndPlayAllSystems();
	}
}