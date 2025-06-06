#pragma once
#include "ECSConfig.h"

namespace ECS
{
	class ISingletonComponent {
	public:
		virtual ~ISingletonComponent() = default;
	};
	template<typename T>
	class SingletonComponentWrapper : public ISingletonComponent {
	public:
		T data;
	};

	class IComponentArray {
	public:
		virtual ~IComponentArray() = default;
		virtual void EntityDestroyed(Entity entity) = 0;
	};

	template<typename T>
	class ComponentArray : public IComponentArray {
		std::array<T, MAX_ENTITIES> mComponentArray;
		std::unordered_map<Entity, size_t> mEntityToIndexMap;
		std::unordered_map<size_t, Entity> mIndexToEntityMap;
		size_t mSize = 0;

	public:
		void InsertData(Entity entity, T component)
		{
			assert(mEntityToIndexMap.find(entity) == mEntityToIndexMap.end() && "Component added to same entity more than once.");

			size_t newIndex = mSize;
			mEntityToIndexMap[entity] = newIndex;
			mIndexToEntityMap[newIndex] = entity;
			mComponentArray[newIndex] = component;
			++mSize;
		}

		void RemoveData(Entity entity)
		{
			assert(mEntityToIndexMap.find(entity) != mEntityToIndexMap.end() && "Removing non-existent component.");

			// Copy element at end into deleted element's place to maintain density
			size_t indexOfRemovedEntity = mEntityToIndexMap[entity];
			size_t indexOfLastElement = mSize - 1;
			mComponentArray[indexOfRemovedEntity] = mComponentArray[indexOfLastElement];

			// Update map to point to moved spot
			Entity entityOfLastElement = mIndexToEntityMap[indexOfLastElement];
			mEntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
			mIndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

			mEntityToIndexMap.erase(entity);
			mIndexToEntityMap.erase(indexOfLastElement);

			--mSize;
		}

		T& GetData(Entity entity)
		{
			assert(mEntityToIndexMap.find(entity) != mEntityToIndexMap.end() && "Retrieving non-existent component.");

			// Return a reference to the entity's component
			return mComponentArray[mEntityToIndexMap[entity]];
		}

		void EntityDestroyed(Entity entity) override
		{
			if (mEntityToIndexMap.find(entity) != mEntityToIndexMap.end())
			{
				// Remove the entity's component if it existed
				RemoveData(entity);
			}
		}
	};

	class ComponentManager
	{
	public:
		template<typename T>
		void RegisterComponent()
		{
			std::type_index type = std::type_index(typeid(T));

			assert(mComponentTypes.find(type) == mComponentTypes.end() && "Registering component type more than once.");

			mComponentTypes.insert({ type, mNextComponentType });
			mComponentArrays.insert({ type, std::make_shared<ComponentArray<T>>() });
			++mNextComponentType;
		}

		template<typename T>
		void RegisterSingletonComponent()
		{
			std::type_index type = std::type_index(typeid(T));

			assert(mSingletonComponents.find(type) == mSingletonComponents.end() && "Singleton already registered.");

			mSingletonComponents[type] = std::make_unique<SingletonComponentWrapper<T>>();
		}

		template<typename T>
		ComponentType GetComponentType()
		{
			std::type_index type = std::type_index(typeid(T));

			assert(mComponentTypes.find(type) != mComponentTypes.end() && "Component not registered before use.");

			return mComponentTypes[type];
		}

		template<typename T>
		void AddComponent(Entity entity, T component)
		{
			GetComponentArray<T>()->InsertData(entity, component);
		}

		template<typename T>
		void RemoveComponent(Entity entity)
		{
			GetComponentArray<T>()->RemoveData(entity);
		}

		template<typename T>
		T& GetComponent(Entity entity)
		{
			return GetComponentArray<T>()->GetData(entity);
		}

		template<typename T>
		const T& GetComponent(Entity entity) const
		{
			return GetComponentArray<T>()->GetData(entity);
		}

		template<typename T>
		T& GetSingletonComponent() {
			std::type_index type = typeid(T);
			assert(mSingletonComponents.find(type) != mSingletonComponents.end());
			return static_cast<SingletonComponentWrapper<T>*>(mSingletonComponents[type].get())->data;
		}

		template<typename T>
		const T& GetSingletonComponent() const
		{
			std::type_index type = typeid(T);
			assert(mSingletonComponents.find(type) != mSingletonComponents.end() && "Singleton not registered.");
			return *static_cast<const SingletonComponentWrapper<T>*>(mSingletonComponents.at(type).get())->data;
		}

		void EntityDestroyed(Entity entity)
		{
			for (auto const& pair : mComponentArrays)
			{
				auto const& component = pair.second;

				component->EntityDestroyed(entity);
			}
		}

	private:
		std::unordered_map<std::type_index, ComponentType> mComponentTypes{};
		std::unordered_map<std::type_index, std::shared_ptr<IComponentArray>> mComponentArrays{};
		std::unordered_map<std::type_index, std::unique_ptr<ISingletonComponent>> mSingletonComponents;

		ComponentType mNextComponentType{};

		template<typename T>
		std::shared_ptr<ComponentArray<T>> GetComponentArray()
		{
			std::type_index type = std::type_index(typeid(T));

			assert(mComponentTypes.find(type) != mComponentTypes.end() && "Component not registered before use.");

			return std::static_pointer_cast<ComponentArray<T>>(mComponentArrays[type]);
		}
	};
}