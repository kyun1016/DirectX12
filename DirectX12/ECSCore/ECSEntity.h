#pragma once
#include "ECSConfig.h"

namespace ECS
{
	class EntityManager
	{
	public:
		EntityManager()
		{
			for (Entity entity = 0; entity < MAX_ENTITIES; ++entity)
				mAvailableEntities.push(entity);
		}

		Entity CreateEntity()
		{
			assert(!mAvailableEntities.empty() && "Too many entities in existence.");

			Entity id = mAvailableEntities.front();
			mAvailableEntities.pop();
			return id;
		}

		void DestroyEntity(Entity entity)
		{
			assert(entity < MAX_ENTITIES && "Entity out of range.");

			mSignatures[entity].reset();
			mAvailableEntities.push(entity);
		}

		void SetSignature(Entity entity, const Signature& signature)
		{
			assert(entity < MAX_ENTITIES && "Entity out of range.");

			mSignatures[entity] = signature;
		}

		Signature GetSignature(Entity entity)
		{
			assert(entity < MAX_ENTITIES && "Entity out of range.");

			return mSignatures[entity];
		}

	private:
		std::queue<Entity> mAvailableEntities{};
		std::array<Signature, static_cast<size_t>(MAX_ENTITIES)> mSignatures{};
	};
}