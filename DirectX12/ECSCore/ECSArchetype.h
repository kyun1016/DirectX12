#pragma once
#include "ECSConfig.h"
#include "ECSSharedComponents.h" 

namespace ECS {

    // IComponentArray는 이제 엔티티가 아닌 인덱스로 데이터를 이동/제거하는 역할을 합니다.
    class IComponentArray {
    public:
        virtual ~IComponentArray() = default;
        virtual void MoveData(ComponentHandle handle, IComponentArray* destArray) = 0;
        virtual void RemoveData(ComponentHandle handle) = 0;
        virtual void AddComponentToJson(ComponentHandle handle, json& jsonObject) const = 0;
    };

    template<typename T>
    class ComponentArray : public IComponentArray {
    private:
        std::vector<T> mComponentArray;
    public:
        ComponentArray() {mComponentArray.reserve(64);}
        size_t Size() const { return mComponentArray.size(); }
        size_t Capacity() const { return mComponentArray.capacity(); }
        void AddData(const T& component) { mComponentArray.emplace_back(component); }
        T& GetData(ComponentHandle handle) {
            assert(handle < mComponentArray.size());
            return mComponentArray[handle];
        }
        const T& GetData(ComponentHandle handle) const {
            assert(handle < mComponentArray.size());
            return mComponentArray[handle];
        }
        void MoveData(ComponentHandle handle, IComponentArray* destArray) override {
            auto* dest = static_cast<ComponentArray<T>*>(destArray);
            dest->AddData(mComponentArray[handle]);
        }
        void RemoveData(ComponentHandle handle) override {
            assert(handle < mComponentArray.size());
            mComponentArray[handle] = std::move(mComponentArray.back());
            mComponentArray.pop_back();
        }
        void AddComponentToJson(ComponentHandle handle, json& jsonObject) const override {
            assert(handle < mComponentArray.size());
            const T& component = mComponentArray[handle];
            jsonObject[T::GetName()] = component;
        }
    };
    class ArchetypeManager;
    class Archetype {
    private:
        ArchetypeManager* const mManager;
        const Signature mSignature;
        const ComponentHandle mSharedComponentId;
        std::vector<Entity> mEntities;
        std::unordered_map<ComponentType, std::unique_ptr<IComponentArray>> mComponentArrays;
        std::unordered_map<Entity, ComponentHandle> mEntityToComponentMap;
    public:
        Archetype(Signature signature, SharedComponentID sharedId, ArchetypeManager* manager);

        const Signature& GetSignature() const { return mSignature; }
        SharedComponentID GetSharedComponentId() const { return mSharedComponentId; }
        size_t GetEntityCount() const { return mEntities.size(); }
		Entity GetEntity(ComponentHandle handle) const {
			assert(handle < mEntities.size() && "Index out of bounds.");
			return mEntities[handle];
		}
        ComponentHandle AddEntity(Entity entity) {
            assert(mEntityToComponentMap.find(entity) == mEntityToComponentMap.end());
            ComponentHandle newIndex = mEntities.size();
            mEntityToComponentMap[entity] = newIndex;
            mEntities.emplace_back(entity);
            return newIndex;
        }
        Entity RemoveEntity(Entity entity) {
            assert(mEntityToComponentMap.find(entity) != mEntityToComponentMap.end());
            for (auto const& [type, array] : mComponentArrays)
                array->RemoveData(mEntityToComponentMap[entity]);
            Entity& movedEntity = mEntities[mEntityToComponentMap[entity]];
            movedEntity = mEntities.back();
            mEntities.pop_back();
            mEntityToComponentMap[movedEntity] = mEntityToComponentMap[entity];
            mEntityToComponentMap.erase(entity);

            return movedEntity == entity ? INVALID_ENTITY : movedEntity;
        }
        template<typename T>
        void AddComponentData(T component) { GetComponentArray<T>()->AddData(component); }
        template<typename T>
        T& GetComponentData(ComponentHandle handle) { return GetComponentArray<T>()->GetData(handle); }
        template<typename T>
        T& GetComponentData(Entity entity) { return GetComponentArray<T>()->GetData(mEntityToComponentMap[entity]); }
        template<typename T>
        ComponentArray<T>* GetComponentArray();
        IComponentArray* GetComponentArray(ComponentType type) {
            assert(mComponentArrays.count(type));
            return mComponentArrays[type].get();
        }
    };

    // 엔티티의 위치를 나타내는 구조체
    struct EntityLocation {
        class Archetype* Archetype = nullptr;
        ComponentHandle Handle = 0;
    };

    // ArchetypeManager: 아키타입을 생성하고 관리
    class ArchetypeManager {
    private:
        std::unique_ptr<SharedComponentManager> mSharedComponentManager;
        std::unordered_map<Signature, std::unordered_map<SharedComponentID, std::unique_ptr<Archetype>>> mArchetypes;
        std::unordered_map<Entity, EntityLocation> mEntityLocations;
        std::unordered_map<std::type_index, ComponentType> mComponentTypes;
        std::unordered_map<ComponentType, std::function<std::unique_ptr<IComponentArray>()>> mComponentFactories;
        ComponentType mNextComponentType = 0;
    public:
        ArchetypeManager() {
            mSharedComponentManager = std::make_unique<SharedComponentManager>();
            mSharedComponentManager->RegisterSharedComponent<SharedRenderProperties, SharedRenderPropertiesHasher>();
        }
        std::unique_ptr<IComponentArray> CreateComponentArray(ComponentType type) {
            assert(mComponentFactories.count(type) > 0 && "Component factory not registered for this type.");
            return mComponentFactories.at(type)();
        }
        template<typename T>
        void RegisterComponent() {
            std::type_index type = std::type_index(typeid(T));
            assert(mComponentTypes.find(type) == mComponentTypes.end());
            mComponentTypes[type] = mNextComponentType;
            mComponentFactories[mNextComponentType] = []() { return std::make_unique<ComponentArray<T>>(); };
            mNextComponentType++;
        }
        template<typename T>
        ComponentType GetComponentType() {
            std::type_index type = std::type_index(typeid(T));
            assert(mComponentTypes.find(type) != mComponentTypes.end());
            return mComponentTypes[type];
        }
        std::vector<Archetype*> GetAllArchetypes() {
            std::vector<Archetype*> archetypes;
            for (auto const& [signature, sharedMap] : mArchetypes) {
                for (auto const& [sharedId, archetypePtr] : sharedMap) {
                    archetypes.push_back(archetypePtr.get());
                }
            }
            return archetypes;
        }
        template<typename T>
        void AddComponent(Entity entity, const T& component, const Signature& oldSignature) {
            EntityLocation oldLocation = { nullptr, 0 };
            if (mEntityLocations.count(entity)) {
                oldLocation = mEntityLocations[entity];
            }
            Signature newSignature = oldSignature;
            newSignature.set(GetComponentType<T>());
            size_t sharedId = oldLocation.Archetype ? oldLocation.Archetype->GetSharedComponentId() : 0;
            Archetype* newArchetype = FindOrCreateArchetype(newSignature, sharedId);
            Archetype* oldArchetype = oldLocation.Archetype;
            if (oldArchetype && oldArchetype != newArchetype) {
                MoveEntityBetweenArchetypes(entity, oldArchetype, newArchetype);
            }
            else {
                ComponentHandle newHandle = newArchetype->AddEntity(entity);
                mEntityLocations[entity] = { newArchetype, newHandle };
            }
            newArchetype->AddComponentData<T>(component);
        }
        template<typename T>
        void RemoveComponent(Entity entity)
        {
            // 1. 엔티티의 현재 위치(아키타입, 인덱스)를 찾는다.
            assert(mEntityLocations.count(entity));
            EntityLocation& oldLocation = mEntityLocations[entity];
            Archetype* sourceArchetype = oldLocation.Archetype;

            // 2. 현재 시그니처에서 T 컴포넌트 비트를 끈 새로운 시그니처를 계산한다.
            Signature newSignature = sourceArchetype->GetSignature();
            newSignature.reset(GetComponentType<T>());

            // 3. 공유 컴포넌트 ID는 그대로 유지하면서, 이사 갈 목적지 아키타입을 찾는다.
            SharedComponentID sharedId = sourceArchetype->GetSharedComponentId();
            Archetype* destinationArchetype = FindOrCreateArchetype(newSignature, sharedId);

            // 4. 엔티티를 기존 아키타입에서 새 아키타입으로 이동시킨다.
            // 이 함수는 데이터 복사, 기존 위치에서 제거, 위치 정보 업데이트를 모두 처리합니다.
            MoveEntityBetweenArchetypes(entity, sourceArchetype, destinationArchetype);
        }

        void EntityDestroyed(Entity entity)
        {
            // 1. 파괴할 엔티티가 어디에 있는지 찾는다.
            assert(mEntityLocations.count(entity));
            EntityLocation& location = mEntityLocations[entity];
            Archetype* archetype = location.Archetype;
            ComponentHandle indexOfRemoved = location.Handle;

            // 2. 해당 아키타입에서 엔티티를 제거한다.
            // 이 과정에서 마지막 요소에 있던 다른 엔티티가 빈자리로 이동(swap)된다.
            Entity movedEntity = archetype->RemoveEntity(entity);

            // 3. mEntityLocations 맵에서 파괴된 엔티티 정보를 완전히 삭제한다.
            mEntityLocations.erase(entity);

            // 4. 만약 swap으로 인해 다른 엔티티의 위치가 변경되었다면,
            //    그 엔티티의 위치 정보(인덱스)를 업데이트해준다.
            if (movedEntity != INVALID_ENTITY) {
                mEntityLocations[movedEntity].Handle = indexOfRemoved;
            }
        }

        template<typename T>
        T& GetComponent(Entity entity) {
            assert(mEntityLocations.find(entity) != mEntityLocations.end());
            EntityLocation& location = mEntityLocations[entity];
            return location.Archetype->GetComponentData<T>(location.Handle);
        }

        template<typename T, typename Hasher = std::hash<T>>
        void SetSharedComponent(Entity entity, const T& props) {
            // 1. 범용 관리자로부터 새로운 공유 컴포넌트 ID를 얻습니다.
            size_t newSharedId = mSharedComponentManager->GetId<T, Hasher>(props);

            // ... (엔티티를 새 아키타입으로 이동시키는 로직은 동일) ...
            // MoveEntityToNewArchetype(...);
        }

        Archetype* FindOrCreateArchetype(const Signature& signature, size_t sharedId) {
            if (mArchetypes[signature].find(sharedId) == mArchetypes[signature].end()) {
                mArchetypes[signature][sharedId] = std::make_unique<Archetype>(signature, sharedId, this);
            }
            return mArchetypes[signature][sharedId].get();
        }

    private:
        void MoveEntityBetweenArchetypes(Entity entity, Archetype* source, Archetype* destination) {
            assert(source != destination);

            ComponentHandle oldIndex = mEntityLocations[entity].Handle;
            ComponentHandle newIndex = destination->AddEntity(entity);

            // 1. 모든 기존 컴포넌트 데이터를 새 아키타입으로 복사
            const auto& sourceSignature = source->GetSignature();
            for (ComponentType i = 0; i < MAX_COMPONENTS; ++i) {
                if (sourceSignature.test(i)) {
                    IComponentArray* sourceArray = source->GetComponentArray(i);
                    IComponentArray* destArray = destination->GetComponentArray(i); // 필요 시 생성
                    sourceArray->MoveData(oldIndex, destArray);
                }
            }

            Entity movedEntity = source->RemoveEntity(entity);
            mEntityLocations[entity] = { destination, newIndex };
            if (source->GetEntityCount() > 0 && movedEntity != entity) {
                mEntityLocations[movedEntity].Handle = oldIndex;
            }
        }
    };
    inline Archetype::Archetype(Signature signature, SharedComponentID sharedId, ArchetypeManager* manager)
        : mSignature(signature)
        , mSharedComponentId(sharedId)
        , mManager(manager)
    {
        for (ComponentType i = 0; i < MAX_COMPONENTS; ++i) {
            if (mSignature.test(i)) {
                mComponentArrays[i] = mManager->CreateComponentArray(i);
            }
        }
    }
    template<typename T>
    inline ComponentArray<T>* Archetype::GetComponentArray() {
        ComponentType type = mManager->GetComponentType<T>();
        assert(mComponentArrays.count(type) > 0 && "Archetype should have this component array.");
        return static_cast<ComponentArray<T>*>(mComponentArrays.at(type).get());
    }
}

namespace ECS {
    class ISingletonComponent {
    public:
        virtual ~ISingletonComponent() = default;
    };
    template<typename T>
    class SingletonComponentWrapper : public ISingletonComponent {
    public:
        T data;
    };
    class SingletonComponentManager {
    public:
        template<typename T>
        void RegisterComponent()
        {
            std::type_index type = std::type_index(typeid(T));

            assert(mSingletonComponents.find(type) == mSingletonComponents.end() && "Singleton already registered.");

            mSingletonComponents[type] = std::make_unique<SingletonComponentWrapper<T>>();
        }

        template<typename T>
        T& GetComponent() {
            std::type_index type = typeid(T);
            assert(mSingletonComponents.find(type) != mSingletonComponents.end());
            return static_cast<SingletonComponentWrapper<T>*>(mSingletonComponents[type].get())->data;
        }

        template<typename T>
        const T& GetComponent() const
        {
            std::type_index type = typeid(T);
            assert(mSingletonComponents.find(type) != mSingletonComponents.end() && "Singleton not registered.");
            return static_cast<const SingletonComponentWrapper<T>*>(mSingletonComponents.at(type).get())->data;
        }

    private:
        std::unordered_map<std::type_index, std::unique_ptr<ISingletonComponent>> mSingletonComponents;
    };
}