#pragma once
#include "pch.h"

#include "ECSEntity.h"
#include "ECSArchetype.h"
#include "ECSSystem.h"
#include "WindowSystem.h"
#include "ImGuiSystem.h"
#include "BoundingVolumeUpdateSystem.h"
#include "PlayerControlSystem.h"
#include "RenderDataSyncSystem.h"

namespace ECS
{
	void Coordinator::Init()
	{
		// Create pointers to each manager
		mArchetypeManager = std::make_unique<ArchetypeManager>();
		mSingletonComponentManager = std::make_unique<SingletonComponentManager>();
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
		RegisterComponent<eCFGInstanceComponent>();
		RegisterComponent<PlayerControlComponent>();

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
		{
			RegisterSystem<PlayerControlSystem>();
			ECS::Signature signature;
			signature.set(GetComponentType<TransformComponent>());
			signature.set(GetComponentType<RigidBodyComponent>());
			signature.set(GetComponentType<PlayerControlComponent>());
			SetSystemSignature<PlayerControlSystem>(signature);
		}
		{
			RegisterSystem<RenderDataSyncSystem>();
			ECS::Signature signature;
			signature.set(GetComponentType<TransformComponent>());
			signature.set(GetComponentType<RigidBodyComponent>());
			signature.set(GetComponentType<PlayerControlComponent>());
			SetSystemSignature<RenderDataSyncSystem>(signature);
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
		mArchetypeManager->EntityDestroyed(entity);
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
		SaveWorldToFile("world.json"); // Save the world state to a file at the end of the game loop
	}
	void Coordinator::SaveWorldToFile(const std::string& filePath)
	{
		json worldJson;

		// 1. 싱글턴 컴포넌트 저장
		//    (Time, GameProgress 등 저장해야 할 싱글턴들을 명시적으로 저장)
		auto& time = GetSingletonComponent<TimeComponent>();
		worldJson = json{
			{"time", {time.deltaTime, time.totalTime}}
		};

		// 2. 리소스 경로 저장
		//    (각 리소스 관리자에게 현재 로드된 리소스 경로 목록을 요청)
		// auto& geoManager = GetSingletonComponent<GeometryManager>();
		// worldJson["Resources"]["Geometries"] = geoManager.GetLoadedPaths(); // 이런 함수가 필요

		//// 3. 모든 아키타입과 엔티티 순회
		//json entitiesJson = json::array();
		//// Coordinator는 모든 아키타입에 접근할 수 있는 인터페이스를 제공해야 함
		//for (Archetype* archetype : GetAllArchetypes()) {

		//	// 3-1. 공유 컴포넌트 데이터 저장
		//	size_t sharedId = archetype->GetSharedComponentId();
		//	// SharedComponentManager를 통해 ID에 해당하는 실제 값을 가져와 JSON으로 변환
		//	const auto& sharedProps = GetSharedComponentValue(sharedId);
		//	json sharedJson = sharedProps;

		//	// 3-2. 아키타입 내 모든 엔티티 순회
		//	for (size_t i = 0; i < archetype->GetEntityCount(); ++i) {
		//		Entity entity = archetype->GetEntity(i); // 가상의 함수
		//		json entityJson;
		//		entityJson["ID"] = entity;
		//		entityJson["SharedComponentID"] = sharedId; // 어떤 공유 데이터를 쓰는지 저장

		//		// 3-3. 엔티티의 모든 일반 컴포넌트 저장
		//		json componentsJson;
		//		const auto& signature = archetype->GetSignature();
		//		for (ComponentType typeId = 0; typeId < MAX_COMPONENTS; ++typeId) {
		//			if (signature.test(typeId)) {
		//				// 타입 ID를 컴포넌트 이름으로 변환하고, 해당 컴포넌트 데이터를 JSON으로 변환
		//				// 이 과정은 리플렉션이 없으므로, 각 타입에 대한 저장 로직을 등록하는 방식이 필요
		//				// 예: mSaveFunctions[typeId](entity, componentsJson);
		//			}
		//		}
		//		entityJson["Components"] = componentsJson;
		//		entitiesJson.push_back(entityJson);
		//	}
		//}
		//worldJson["Entities"] = entitiesJson;

		// 4. 최종 JSON을 파일에 쓰기
		std::ofstream file(filePath);
		file << worldJson.dump(4); // 4는 들여쓰기 옵션
	}
	void Coordinator::LoadWorldFromFile(const std::string& filePath)
	{
		//auto& coordinator = ECS::Coordinator::GetInstance();

		//// 1. 기존 월드 초기화 (모든 엔티티, 컴포넌트 삭제)
		//coordinator.DestroyAllEntities();

		//// 2. JSON 파일 읽기
		//std::ifstream file(filePath);
		//json worldJson;
		//file >> worldJson;

		//// 3. 리소스 미리 로드하기
		////    (엔티티 생성 전에 필요한 모든 리소스를 로드하여 핸들을 준비)
		//auto& geoManager = coordinator.GetSingletonComponent<GeometryManager>();
		//for (const auto& path : worldJson["Resources"]["Geometries"]) {
		//	geoManager.LoadFromFile(path.get<std::string>(), device);
		//}

		//// 4. 싱글턴 컴포넌트 복원
		//coordinator.GetSingletonComponent<TimeComponent>() = worldJson["Singletons"]["Time"];

		//// 5. 엔티티 및 컴포넌트 생성
		//for (const auto& entityJson : worldJson["Entities"]) {
		//	// 5-1. 엔티티 생성
		//	Entity entity = coordinator.CreateEntity(entityJson["ID"]); // ID를 지정하여 생성

		//	// 5-2. 공유 컴포넌트 설정
		//	size_t sharedId = entityJson["SharedComponentID"];
		//	const auto& sharedProps = coordinator.GetSharedComponentValue(sharedId);
		//	coordinator.SetSharedComponent(entity, sharedProps);

		//	// 5-3. 일반 컴포넌트 추가
		//	for (auto& [compName, compJson] : entityJson["Components"].items()) {
		//		// 컴포넌트 이름(compName)을 보고 어떤 타입인지 판단하여
		//		// from_json 함수로 데이터를 채운 뒤, AddComponent 호출
		//		// 예: mLoadFunctions[compName](entity, compJson);
		//	}
		//}
	}
}