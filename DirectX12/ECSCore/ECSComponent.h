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
		std::unordered_map<Entity, size_t> mEntityToComponentMap;
		std::unordered_map<size_t, Entity> mIndexToEntityMap;
		size_t mSize = 0;
	public:
		void AddData(Entity entity, T component)
		{
			assert(mEntityToComponentMap.find(entity) == mEntityToComponentMap.end() && "Component added to same entity more than once.");

			mEntityToComponentMap[entity] = mSize;
			mIndexToEntityMap[mSize] = entity;
			mComponentArray[mSize] = component;
			++mSize;
		}

		void RemoveData(Entity entity)
		{
			assert(mEntityToComponentMap.find(entity) != mEntityToComponentMap.end() && "Removing non-existent component.");

			// Copy element at end into deleted element's place to maintain density
			--mSize;
			size_t& indexOfRemovedEntity = mEntityToComponentMap[entity];
			mComponentArray[indexOfRemovedEntity] = mComponentArray[mSize];

			// Update map to point to moved spot
			Entity& entityOfLastElement = mIndexToEntityMap[mSize];
			mEntityToComponentMap[entityOfLastElement] = indexOfRemovedEntity;
			mIndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;

			mEntityToComponentMap.erase(entity);
			mIndexToEntityMap.erase(mSize);
		}

		T& GetData(Entity entity)
		{
			assert(mEntityToComponentMap.find(entity) != mEntityToComponentMap.end() && "Retrieving non-existent component.");

			// Return a reference to the entity's component
			return mComponentArray[mEntityToComponentMap[entity]];
		}

		void EntityDestroyed(Entity entity) override
		{
			if (mEntityToComponentMap.find(entity) != mEntityToComponentMap.end())
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
		ComponentArray<T>* GetComponentArray()
		{
			std::type_index type = std::type_index(typeid(T));

			assert(mComponentTypes.find(type) != mComponentTypes.end() && "Component not registered before use.");

			return static_cast<ComponentArray<T>*>(mComponentArrays[type].get());
		}

		template<typename T>
		void RegisterComponent()
		{
			std::type_index type = std::type_index(typeid(T));

			assert(mComponentTypes.find(type) == mComponentTypes.end() && "Registering component type more than once.");

			mComponentTypes.insert({ type, mNextComponentType });
			mComponentArrays.insert({ type, std::make_unique<ComponentArray<T>>() });
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
			GetComponentArray<T>()->AddData(entity, component);
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
		std::unordered_map<std::type_index, std::unique_ptr<IComponentArray>> mComponentArrays{};
		std::unordered_map<std::type_index, std::unique_ptr<ISingletonComponent>> mSingletonComponents;

		ComponentType mNextComponentType{};
	};
}