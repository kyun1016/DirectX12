#pragma once
#include "ECSConfig.h"

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
    // 아키타입: 동일한 컴포넌트 조합을 가진 엔티티들의 집합
    class Archetype {
    private:
        Signature mSignature;
        // 이 아키타입이 소유한 컴포넌트 배열들
        std::unordered_map<ComponentType, std::unique_ptr<IComponentArray>> mComponentArrays;
        // 이 아키타입에 속한 엔티티 목록
        std::vector<Entity> mEntities;
        // Entity ID -> 배열 인덱스 맵
        std::unordered_map<Entity, size_t> mEntityToIndexMap;

    public:
        Archetype(Signature signature) : mSignature(signature) {}

        const Signature& GetSignature() const { return mSignature; }
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
            Entity movedEntity = mEntities[indexOfRemoved];
            mEntityToIndexMap[movedEntity] = indexOfRemoved;
            mEntityToIndexMap.erase(entity);

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
        ComponentArray<T>* GetComponentArray() {
            ComponentType type = GetComponentType<T>(); // 전역 함수 또는 Coordinator를 통해 가져와야 함
            if (mComponentArrays.find(type) == mComponentArrays.end()) {
                mComponentArrays[type] = std::make_unique<ComponentArray<T>>();
            }
            return static_cast<ComponentArray<T>*>(mComponentArrays[type].get());
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
        // 엔티티의 위치 정보
        std::unordered_map<Entity, EntityLocation> mEntityLocations;
        // 시그니처 -> 아키타입 맵
        std::unordered_map<Signature, std::unique_ptr<Archetype>> mArchetypes;
        // 컴포넌트 타입 등록 정보
        std::unordered_map<std::type_index, ComponentType> mComponentTypes;
        ComponentType mNextComponentType = 0;

    public:
        template<typename T>
        void RegisterComponent() {
            std::type_index type = std::type_index(typeid(T));
            assert(mComponentTypes.find(type) == mComponentTypes.end());
            mComponentTypes[type] = mNextComponentType++;
        }

        template<typename T>
        ComponentType GetComponentType() {
            std::type_index type = std::type_index(typeid(T));
            assert(mComponentTypes.find(type) != mComponentTypes.end());
            return mComponentTypes[type];
        }

        template<typename T>
        void AddComponent(Entity entity, T component, Signature oldSignature) {
            // 1. 새로운 시그니처 계산
            Signature newSignature = oldSignature;
            newSignature.set(GetComponentType<T>());

            // 2. 기존 위치와 새 아키타입 찾기
            EntityLocation& oldLocation = mEntityLocations[entity];
            Archetype* oldArchetype = oldLocation.archetype;
            Archetype* newArchetype = FindOrCreateArchetype(newSignature);

            // 3. 새 아키타입에 엔티티를 위한 공간 할당 및 데이터 이동
            size_t newIndex = newArchetype->AddEntity(entity);
            // 모든 기존 컴포넌트 데이터를 새 아키타입으로 복사
            for (ComponentType i = 0; i < MAX_COMPONENTS; ++i) {
                if (oldSignature.test(i)) {
                    // ... (데이터 이동 로직 구현 필요) ...
                }
            }
            // 새 컴포넌트 데이터 추가
            newArchetype->AddComponentData(component);

            // 4. 기존 아키타입에서 엔티티 제거
            Entity movedEntity = oldArchetype->RemoveEntity(entity);

            // 5. 위치 정보 업데이트
            mEntityLocations[entity] = { newArchetype, newIndex };
            if (oldArchetype->GetEntityCount() > 0) { // 이동된 엔티티가 있다면
                mEntityLocations[movedEntity].index = oldLocation.index;
            }
        }

        template<typename T>
        T& GetComponent(Entity entity) {
            assert(mEntityLocations.find(entity) != mEntityLocations.end());
            EntityLocation& location = mEntityLocations[entity];
            return location.archetype->GetComponentData<T>(location.index);
        }

    private:
        Archetype* FindOrCreateArchetype(const Signature& signature) {
            if (mArchetypes.find(signature) == mArchetypes.end()) {
                mArchetypes[signature] = std::make_unique<Archetype>(signature);
            }
            return mArchetypes[signature].get();
        }
    };
}