#pragma once
#include "ECSConfig.h"
#include "ECSSharedComponents.h" 

namespace ECS {

    // IComponentArray는 이제 엔티티가 아닌 인덱스로 데이터를 이동/제거하는 역할을 합니다.
    class IComponentArray {
    public:
        virtual ~IComponentArray() = default;
        // 한 아키타입에서 다른 아키타입으로 엔티티가 이동할 때 호출됩니다.
        virtual void MoveData(size_t sourceIndex, IComponentArray* destinationArray) = 0;
        // 아키타입에서 엔티티가 제거될 때 호출됩니다.
        virtual void RemoveData(size_t index) = 0;
    };

    // ComponentArray는 이제 각 아키타입 내에서 실제 데이터를 저장하는 역할을 합니다.
    // 더 이상 Entity-to-Index 매핑을 직접 관리하지 않습니다.
    template<typename T>
    class ComponentArray : public IComponentArray {
    private:
        // std::array 대신 std::vector를 사용하여 동적으로 크기를 조절합니다.
        std::vector<T> mComponentArray;

    public:
        // 새로운 데이터를 배열 끝에 추가합니다.
        void InsertData(T component) {
            mComponentArray.push_back(component);
        }

        // 특정 인덱스의 데이터를 가져옵니다.
        T& GetData(size_t index) {
            assert(index < mComponentArray.size() && "Index out of bounds.");
            return mComponentArray[index];
        }

        // 데이터를 다른 ComponentArray로 이동시킵니다.
        void MoveData(size_t sourceIndex, IComponentArray* destinationArray) override {
            auto* dest = static_cast<ComponentArray<T>*>(destinationArray);
            dest->InsertData(mComponentArray[sourceIndex]);
        }

        // 'swap-and-pop' 기법으로 데이터를 제거하여 밀집도를 유지합니다.
        void RemoveData(size_t index) override {
            assert(index < mComponentArray.size() && "Index out of bounds.");
            // 제거할 요소와 마지막 요소를 교환
            mComponentArray[index] = mComponentArray.back();
            // 마지막 요소 제거
            mComponentArray.pop_back();
        }
    };
}

namespace ECS {
    class ArchetypeManager; // 전방 선언

    // 아키타입: 동일한 컴포넌트 조합을 가진 엔티티들의 집합
    class Archetype {
    private:
        Signature mSignature;
        size_t mSharedComponentId;
        // 이 아키타입이 소유한 컴포넌트 배열들
        std::unordered_map<ComponentType, std::unique_ptr<IComponentArray>> mComponentArrays;
        // 이 아키타입에 속한 엔티티 목록
        std::vector<Entity> mEntities;
        // Entity ID -> 배열 인덱스 맵
        std::unordered_map<Entity, size_t> mEntityToIndexMap;
        ArchetypeManager* mManager;

    public:
        Archetype(Signature signature, size_t sharedId, ArchetypeManager* manager);

        const Signature& GetSignature() const { return mSignature; }
		size_t GetSharedComponentId() const { return mSharedComponentId; }
        size_t GetEntityCount() const { return mEntities.size(); }

        // 엔티티를 이 아키타입에 추가 (데이터는 별도로 추가됨)
        size_t AddEntity(Entity entity) {
            assert(mEntityToIndexMap.find(entity) == mEntityToIndexMap.end());
            size_t newIndex = mEntities.size();
            mEntityToIndexMap[entity] = newIndex;
            mEntities.emplace_back(entity);
            return newIndex;
        }

        // 아키타입에서 엔티티를 제거하고, 이동된 엔티티를 반환
        Entity RemoveEntity(Entity entity) {
            assert(mEntityToIndexMap.find(entity) != mEntityToIndexMap.end());

            size_t indexOfRemoved = mEntityToIndexMap[entity];
            size_t indexOfLast = mEntities.size() - 1;

            // 데이터 배열들에서 'swap-and-pop' 수행
            for (auto const& [type, array] : mComponentArrays) {
                array->RemoveData(indexOfRemoved);
            }

            // 엔티티 목록에서 'swap-and-pop' 수행
            mEntities[indexOfRemoved] = mEntities[indexOfLast];
            mEntities.pop_back();

            // 맵 업데이트
            mEntityToIndexMap.erase(entity);
            if (indexOfLast == indexOfRemoved)
                return 0;

            Entity movedEntity = mEntities[indexOfRemoved];
            mEntityToIndexMap[movedEntity] = indexOfRemoved;
            return movedEntity;
        }

        template<typename T>
        void AddComponentData(T component) {
            GetComponentArray<T>()->InsertData(component);
        }

        template<typename T>
        T& GetComponentData(size_t index) {
            return GetComponentArray<T>()->GetData(index);
        }

        template<typename T>
        ComponentArray<T>* GetComponentArray();

        IComponentArray* GetComponentArray(ComponentType type) {
            assert(mComponentArrays.count(type));
            return mComponentArrays[type].get();
        }
    };

    // 엔티티의 위치를 나타내는 구조체
    struct EntityLocation {
        class Archetype* archetype = nullptr;
        size_t index = 0;
    };

    // ArchetypeManager: 아키타입을 생성하고 관리
    class ArchetypeManager {
    private:
        std::unique_ptr<SharedComponentManager> mSharedComponentManager;
        // { Signature -> { SharedComponentId -> Archetype* } }
        std::unordered_map<Signature, std::unordered_map<size_t, std::unique_ptr<Archetype>>> mArchetypes;
        // 엔티티의 위치 정보
        std::unordered_map<Entity, EntityLocation> mEntityLocations;
        // 컴포넌트 타입 등록 정보
        std::unordered_map<std::type_index, ComponentType> mComponentTypes;
        std::unordered_map<ComponentType, std::function<std::unique_ptr<IComponentArray>()>> mComponentFactories;
        ComponentType mNextComponentType = 0;

    public:
        ArchetypeManager() {
            mSharedComponentManager = std::make_unique<SharedComponentManager>();
            // 사용할 모든 공유 컴포넌트 타입을 미리 등록합니다.
            mSharedComponentManager->RegisterSharedComponent<SharedRenderProperties, SharedRenderPropertiesHasher>();
            // mSharedComponentManager->RegisterSharedComponent<SharedPhysicsProperties, ...>(); // 다른 타입도 등록 가능
        }

        std::unique_ptr<IComponentArray> CreateComponentArray(ComponentType type) {
            assert(mComponentFactories.count(type) > 0 && "Component factory not registered for this type.");
            // 등록된 팩토리 함수를 호출하여 컴포넌트 배열 생성
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

        template<typename T>
        void AddComponent(Entity entity, T component, Signature oldSignature) {
            // 1. 엔티티의 현재 위치와 시그니처를 가져온다.
            //    신규 엔티티라면, 기본값(nullptr, 0)과 빈 시그니처를 사용한다.
            EntityLocation oldLocation = { nullptr, 0 };
            if (mEntityLocations.count(entity)) {
                oldLocation = mEntityLocations[entity];
            }

            // 2. 새로운 시그니처를 계산한다.
            Signature newSignature = oldSignature;
            newSignature.set(GetComponentType<T>());

            // 3. 공유 컴포넌트 ID는 그대로 유지한다. (신규 엔티티는 기본값 0 사용)
            size_t sharedId = oldLocation.archetype ? oldLocation.archetype->GetSharedComponentId() : 0;

            // 4. 이사 갈 목적지 아키타입을 찾거나 생성한다.
            Archetype* newArchetype = FindOrCreateArchetype(newSignature, sharedId);
            Archetype* oldArchetype = oldLocation.archetype;

            // 5. 신규 엔티티가 아니라면, 기존 아키타입에서 새 아키타입으로 데이터를 옮긴다.
            if (oldArchetype && oldArchetype != newArchetype) {
                MoveEntityBetweenArchetypes(entity, oldArchetype, newArchetype);
            }
            // 신규 엔티티라면, 새 아키타입에 바로 추가한다.
            else {
                size_t newIndex = newArchetype->AddEntity(entity);
                mEntityLocations[entity] = { newArchetype, newIndex };
            }

            // 6. 마지막으로, 새로 추가된 컴포넌트의 데이터를 삽입한다.
            newArchetype->AddComponentData<T>(component);
        }

        template<typename T>
        void RemoveComponent(Entity entity)
        {
            // 1. 엔티티의 현재 위치(아키타입, 인덱스)를 찾는다.
            assert(mEntityLocations.count(entity));
            EntityLocation& oldLocation = mEntityLocations[entity];
            Archetype* sourceArchetype = oldLocation.archetype;

            // 2. 현재 시그니처에서 T 컴포넌트 비트를 끈 새로운 시그니처를 계산한다.
            Signature newSignature = sourceArchetype->GetSignature();
            newSignature.reset(GetComponentType<T>());

            // 3. 공유 컴포넌트 ID는 그대로 유지하면서, 이사 갈 목적지 아키타입을 찾는다.
            size_t sharedId = sourceArchetype->GetSharedComponentId();
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
            Archetype* archetype = location.archetype;
            size_t indexOfRemoved = location.index;

            // 2. 해당 아키타입에서 엔티티를 제거한다.
            // 이 과정에서 마지막 요소에 있던 다른 엔티티가 빈자리로 이동(swap)된다.
            Entity movedEntity = archetype->RemoveEntity(entity);

            // 3. mEntityLocations 맵에서 파괴된 엔티티 정보를 완전히 삭제한다.
            mEntityLocations.erase(entity);

            // 4. 만약 swap으로 인해 다른 엔티티의 위치가 변경되었다면,
            //    그 엔티티의 위치 정보(인덱스)를 업데이트해준다.
            if (archetype->GetEntityCount() > 0 && movedEntity != entity) {
                mEntityLocations[movedEntity].index = indexOfRemoved;
            }
        }

        template<typename T>
        T& GetComponent(Entity entity) {
            assert(mEntityLocations.find(entity) != mEntityLocations.end());
            EntityLocation& location = mEntityLocations[entity];
            return location.archetype->GetComponentData<T>(location.index);
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
            if (source == destination) return; // 같은 아키타입이면 이동 불필요

            size_t oldIndex = mEntityLocations[entity].index;
            size_t newIndex = destination->AddEntity(entity);

            // 1. 모든 기존 컴포넌트 데이터를 새 아키타입으로 복사
            const auto& sourceSignature = source->GetSignature();
            for (ComponentType i = 0; i < MAX_COMPONENTS; ++i) {
                if (sourceSignature.test(i)) {
                    IComponentArray* sourceArray = source->GetComponentArray(i);
                    IComponentArray* destArray = destination->GetComponentArray(i); // 필요 시 생성
                    sourceArray->MoveData(oldIndex, destArray);
                }
            }

            // 2. 기존 아키타입에서 엔티티 제거 (이때 다른 엔티티가 이동됨)
            Entity movedEntity = source->RemoveEntity(entity);

            // 3. 위치 정보(Location) 업데이트
            mEntityLocations[entity] = { destination, newIndex };
            if (source->GetEntityCount() > 0 && movedEntity != entity) {
                // 기존 아키타입에서 빈자리로 이동해 온 엔티티의 인덱스 업데이트
                mEntityLocations[movedEntity].index = oldIndex;
            }
        }
    };


    inline Archetype::Archetype(Signature signature, size_t sharedId, ArchetypeManager* manager)
        : mSignature(signature)
        , mSharedComponentId(sharedId)
        , mManager(manager)
    {
        for (ComponentType i = 0; i < MAX_COMPONENTS; ++i) {
            if (mSignature.test(i)) {
                // Manager를 통해 ComponentType에 맞는 배열을 생성하는 팩토리 함수 호출
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
            return *static_cast<const SingletonComponentWrapper<T>*>(mSingletonComponents.at(type).get())->data;
        }

    private:
        std::unordered_map<std::type_index, std::unique_ptr<ISingletonComponent>> mSingletonComponents;
    };
}