#pragma once
#include "pch.h"

#include "ECSEntity.h"
#include "ECSComponent.h"
#include "ECSSystem.h"
#include "WindowSystem.h"
#include "ImGuiSystem.h"
#include "BoundingVolumeUpdateSystem.h"

namespace ECS
{
	void Coordinator::Init()
	{
		// Create pointers to each manager
		mComponentManager = std::make_unique<ComponentManager>();
		mEntityManager = std::make_unique<EntityManager>();
		mSystemManager = std::make_unique<SystemManager>();

		RegisterComponent<DX12_BoundingComponent>();
		RegisterComponent<DX12_MeshComponent>();
		RegisterComponent<InstanceData>();
		RegisterComponent<LightComponent>();
		RegisterComponent<FMODAudioComponent>();
		RegisterComponent<TransformComponent>();
		RegisterComponent<RigidBodyComponent>();
		RegisterComponent<GravityComponent>();
		RegisterComponent<BoundingVolumnComponent>();

		{
			RegisterSystem<FMODAudioSystem>();
			ECS::Signature signature;
			signature.set(GetComponentType<FMODAudioComponent>());
			SetSystemSignature<FMODAudioSystem>(signature);
		}

		{
			RegisterSystem<PhysicsSystem>();
			ECS::Signature signature;
			signature.set(GetComponentType<TransformComponent>());
			signature.set(GetComponentType<RigidBodyComponent>());
			signature.set(GetComponentType<GravityComponent>());
			SetSystemSignature<PhysicsSystem>(signature);
		}

		{
			RegisterSystem<LightSystem>();
			ECS::Signature signature;
			signature.set(GetComponentType<LightComponent>());
			SetSystemSignature<LightSystem>(signature);
		}

		{
			RegisterSystem<DX12_BoundingSystem>();
			ECS::Signature signature;
			signature.set(GetComponentType<TransformComponent>());
			signature.set(GetComponentType<DX12_MeshComponent>());
			signature.set(GetComponentType<DX12_BoundingComponent>());
			SetSystemSignature<DX12_BoundingSystem>(signature);
		}
		{
			RegisterSystem<BoundingVolumeUpdateSystem>();
			ECS::Signature signature;
			signature.set(GetComponentType<TransformComponent>());
			signature.set(GetComponentType<DX12_MeshComponent>());
			signature.set(GetComponentType<BoundingVolumnComponent>());
			SetSystemSignature<BoundingVolumeUpdateSystem>(signature);
		}
		
		
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