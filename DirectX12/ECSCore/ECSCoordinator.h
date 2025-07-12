#pragma once
#include "ECSEntity.h"
#include "ECSComponent.h"
#include "ECSSystem.h"
#include "WindowSystem.h"
#include "DX12_FrameResourceSystem.h"

namespace ECS
{
	class Coordinator
	{
	public:
		Coordinator(const Coordinator&) = delete;
		Coordinator(Coordinator&&) = delete;
		Coordinator& operator=(const Coordinator&) = delete;
		Coordinator& operator=(Coordinator&&) = delete;

		static Coordinator& GetInstance()
		{
			static Coordinator instance;
			return instance;
		}

		void Init()
		{
			// Create pointers to each manager
			mComponentManager = std::make_unique<ComponentManager>();
			mEntityManager = std::make_unique<EntityManager>();
			mSystemManager = std::make_unique<SystemManager>();
		}


		// Entity methods
		Entity CreateEntity()
		{
			return mEntityManager->CreateEntity();
		}

		void DestroyEntity(Entity entity)
		{
			std::lock_guard<std::mutex> lock(mtx);
			mEntityManager->DestroyEntity(entity);
			mComponentManager->EntityDestroyed(entity);
			mSystemManager->EntityDestroyed(entity);
		}

		// Component methods
		template<typename T>
		void RegisterComponent()
		{
			std::lock_guard<std::mutex> lock(mtx);
			mComponentManager->RegisterComponent<T>();
		}


		template<typename T>
		void RegisterSingletonComponent()
		{
			std::lock_guard<std::mutex> lock(mtx);
			mComponentManager->RegisterSingletonComponent<T>();
		}

		template<typename T>
		void AddComponent(Entity entity, T component)
		{
			std::lock_guard<std::mutex> lock(mtx);
			mComponentManager->AddComponent<T>(entity, component);

			auto signature = mEntityManager->GetSignature(entity);
			signature.set(mComponentManager->GetComponentType<T>(), true);
			mEntityManager->SetSignature(entity, signature);

			mSystemManager->EntitySignatureChanged(entity, signature);
		}

		template<typename T>
		void RemoveComponent(Entity entity)
		{
			std::lock_guard<std::mutex> lock(mtx);
			mComponentManager->RemoveComponent<T>(entity);

			auto signature = mEntityManager->GetSignature(entity);
			signature.set(mComponentManager->GetComponentType<T>(), false);
			mEntityManager->SetSignature(entity, signature);

			mSystemManager->EntitySignatureChanged(entity, signature);
		}

		template<typename T>
		T& GetComponent(Entity entity)
		{
			return mComponentManager->GetComponent<T>(entity);
		}

		template<typename T>
		const T& GetComponent(Entity entity) const
		{
			return mComponentManager->GetComponent<T>(entity);
		}

		template<typename T>
		T& GetSingletonComponent()
		{
			return mComponentManager->GetSingletonComponent<T>();
		}

		template<typename T>
		const T& GetSingletonComponent() const
		{
			return mComponentManager->GetSingletonComponent<T>();
		}

		template<typename T>
		ComponentType GetComponentType()
		{
			return mComponentManager->GetComponentType<T>();
		}

		// System methods
		template<typename T>
		std::shared_ptr<T> RegisterSystem()
		{
			return mSystemManager->RegisterSystem<T>();
		}

		template<typename T>
		void SetSystemSignature(Signature signature)
		{
			mSystemManager->SetSignature<T>(signature);
		}

		void UpdateAllSystem()
		{
			mSystemManager->UpdateAllSystems();
		}

		void Run()
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
				DX12_FrameResourceSystem::GetInstance().BeginFrame();
				mSystemManager->PreUpdateAllSystems();
				mSystemManager->UpdateAllSystems();
				DX12_SwapChainSystem::GetInstance().Present(false);
				DX12_FrameResourceSystem::GetInstance().EndFrame();
				ImGuiSystem::GetInstance().RenderMultiViewport();
				mSystemManager->LateUpdateAllSystems();
				mSystemManager->FixedUpdateAllSystems();
				mSystemManager->FinalUpdateAllSystems();
			}
			mSystemManager->EndPlayAllSystems();
		}

	private:
		Coordinator() = default;
		std::mutex mtx;
		std::unique_ptr<EntityManager> mEntityManager;
		std::unique_ptr<ComponentManager> mComponentManager;
		std::unique_ptr<SystemManager> mSystemManager;
	};
}