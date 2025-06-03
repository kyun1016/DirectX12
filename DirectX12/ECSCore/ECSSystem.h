#pragma once
#include "ECSConfig.h"
#include <future>

namespace ECS
{
	class ISystem
	{
	public:
		std::set<Entity> mEntities;
		virtual void Update(float dt) = 0;
		virtual ~ISystem() = default;
	};

	class SystemManager
	{
	public:
		template<typename T>
		std::shared_ptr<T> RegisterSystem()
		{
			std::type_index type = std::type_index(typeid(T));

			assert(mSystems.find(type) == mSystems.end() && "Registering system more than once.");

			// Create a pointer to the system and return it so it can be used externally
			auto system = std::make_shared<T>();
			mSystems.insert({ type, system });
			mSystemTasks[type] = [system](float dt) {
				system->Update(dt);
			};

			return system;
		}

		template<typename T>
		void SetSignature(Signature signature)
		{
			std::type_index type = std::type_index(typeid(T));

			assert(mSystems.find(type) != mSystems.end() && "System used before registered.");

			// Set the signature for this system
			mSignatures.insert({ type, signature });
		}

		void EntityDestroyed(Entity entity)
		{
			// Erase a destroyed entity from all system lists
			// mEntities is a set so no check needed
			for (auto const& pair : mSystems)
			{
				auto const& system = pair.second;

				system->mEntities.erase(entity);
			}
		}

		void EntitySignatureChanged(Entity entity, Signature entitySignature)
		{
			// Notify each system that an entity's signature changed
			for (auto const& pair : mSystems)
			{
				auto const& type = pair.first;
				auto const& system = pair.second;
				auto const& systemSignature = mSignatures[type];

				if ((entitySignature & systemSignature) == systemSignature)
				{
					system->mEntities.insert(entity);
				}
				else
				{
					system->mEntities.erase(entity);
				}
			}
		}

		void UpdateAllSystems(float dt) {
			std::vector<std::future<void>> futures;
			for (auto& [_, task] : mSystemTasks)
			{
				futures.emplace_back(std::async(std::launch::async, task, dt));
			}

			for (auto& fut : futures)
			{
				fut.get(); // 모든 시스템의 업데이트 완료 대기
			}
		}

	private:
		std::unordered_map<std::type_index, Signature> mSignatures{};
		std::unordered_map<std::type_index, std::shared_ptr<ISystem>> mSystems{};
		std::unordered_map<std::type_index, std::function<void(float)>> mSystemTasks;
	};
}