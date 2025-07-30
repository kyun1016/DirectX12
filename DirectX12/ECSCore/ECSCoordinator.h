#pragma once
#include "ECSEntity.h"
#include "ECSArchetype.h"
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

		void SaveWorldToFile(const std::string& filePath);
		void LoadWorldFromFile(const std::string& filePath);

		// Component methods
		template<typename T>
		void RegisterComponent()
		{
			std::lock_guard<std::mutex> lock(mtx);
			mArchetypeManager->RegisterComponent<T>();
		}
		template<typename T>
		void RegisterSingletonComponent()
		{
			std::lock_guard<std::mutex> lock(mtx);
			mSingletonComponentManager->RegisterComponent<T>();
		}
		template<typename T>
		void AddComponent(Entity entity, T component)
		{
			std::lock_guard<std::mutex> lock(mtx);
			auto signature = mEntityManager->GetSignature(entity);
			mArchetypeManager->AddComponent<T>(entity, component, signature);
			signature.set(mArchetypeManager->GetComponentType<T>(), true);
			
			mEntityManager->SetSignature(entity, signature);

			mSystemManager->EntitySignatureChanged(entity, signature);
		}

		template<typename T>
		void RemoveComponent(Entity entity)
		{
			std::lock_guard<std::mutex> lock(mtx);
			mArchetypeManager->RemoveComponent<T>(entity);

			auto signature = mEntityManager->GetSignature(entity);
			signature.set(mArchetypeManager->GetComponentType<T>(), false);
			mEntityManager->SetSignature(entity, signature);

			mSystemManager->EntitySignatureChanged(entity, signature);
		}

		template<typename T>
		T& GetComponent(Entity entity)
		{
			return mArchetypeManager->GetComponent<T>(entity);
		}

		template<typename T>
		const T& GetComponent(Entity entity) const
		{
			return mArchetypeManager->GetComponent<T>(entity);
		}
		
		std::vector<Archetype*> GetAllArchetypes() {
			return mArchetypeManager->GetAllArchetypes();
		}

		template<typename T>
		T& GetSingletonComponent()
		{
			return mSingletonComponentManager->GetComponent<T>();
		}

		template<typename T>
		const T& GetSingletonComponent() const
		{
			return mSingletonComponentManager->GetComponent<T>();
		}

		template<typename T>
		ComponentType GetComponentType()
		{
			return mArchetypeManager->GetComponentType<T>();
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
		std::unique_ptr<ArchetypeManager> mArchetypeManager;
		std::unique_ptr<SingletonComponentManager> mSingletonComponentManager;
		std::unique_ptr<SystemManager> mSystemManager;
	};
}