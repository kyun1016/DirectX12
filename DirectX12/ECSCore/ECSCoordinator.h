#pragma once
#include "ECSEntity.h"
#include "ECSComponent.h"
// #include "ECSArchetype.h"
#include "ECSSystem.h"

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

		void Init();
		Entity CreateEntity();
		void DestroyEntity(Entity entity);
		void Run();

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
	private:
		Coordinator() = default;
		std::mutex mtx;
		std::unique_ptr<EntityManager> mEntityManager;
		std::unique_ptr<ComponentManager> mComponentManager;
		std::unique_ptr<SystemManager> mSystemManager;
	};
}