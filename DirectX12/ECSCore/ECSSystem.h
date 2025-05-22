#pragma once
#include "ECSConfig.h"

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
			for (auto& [_, system] : mSystems) {
				system->Update(dt);  // 순수 가상이므로 하위 시스템에서 구현 강제됨
			}
		}

	private:
		std::unordered_map<std::type_index, Signature> mSignatures{};
		std::unordered_map<std::type_index, std::shared_ptr<ISystem>> mSystems{};
	};
}