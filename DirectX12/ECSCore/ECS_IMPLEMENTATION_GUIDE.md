# ECS 시스템 단계별 구현 가이드

## 📋 목차
1. [구현 개요](#구현-개요)
2. [Step 1: 기본 구조 설계](#step-1-기본-구조-설계)
3. [Step 2: Entity 관리 시스템](#step-2-entity-관리-시스템)
4. [Step 3: Component 시스템](#step-3-component-시스템)
5. [Step 4: Archetype 기반 메모리 관리](#step-4-archetype-기반-메모리-관리)
6. [Step 5: System 관리 및 실행](#step-5-system-관리-및-실행)
7. [Step 6: 고급 기능 구현](#step-6-고급-기능-구현)
8. [Step 7: 최적화 및 디버깅](#step-7-최적화-및-디버깅)

---

## 구현 개요

### 🎯 목표
- **성능 중심**: Archetype 기반 메모리 최적화
- **확장성**: 새로운 컴포넌트/시스템 쉽게 추가
- **안전성**: 타입 안전성과 메모리 안전성 보장
- **실용성**: 실제 게임 개발에 바로 사용 가능

### 📐 아키텍처 개요
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Coordinator   │◄──►│ArchetypeManager │◄──►│    Systems      │
│   (Facade)      │    │  (Data Storage) │    │   (Logic)       │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         ▼                       ▼                       ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│ EntityManager   │    │  ComponentArray │    │ SystemManager   │
│   (ID 관리)      │    │ (실제 데이터)    │    │ (시스템 실행)    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
```

---

## Step 1: 기본 구조 설계

### 📝 **이 단계에서 구현할 것**
- [ ] 기본 타입 정의 (`ECSConfig.h`)
- [ ] Entity 타입 정의 (`ECSEntity.h`)
- [ ] Signature 시스템
- [ ] 기본 인터페이스 클래스들

### 🎯 **목표**
ECS 시스템의 **기반이 되는 타입들과 설정**을 정의합니다.

### 📋 **체크리스트**

#### 1.1 기본 설정 파일 생성
```cpp
// ECSConfig.h - 시스템 전체 설정
#pragma once
#include <bitset>
#include <cstdint>

namespace ECS {
    // 시스템 제한
    static constexpr std::uint32_t MAX_ENTITIES = 50000;    // 최대 엔티티 수
    static constexpr std::uint8_t MAX_COMPONENTS = 64;      // 최대 컴포넌트 타입 수
    static constexpr std::uint8_t MAX_SYSTEMS = 32;         // 최대 시스템 수
    
    // 기본 타입들
    using Entity = std::uint32_t;                           // 엔티티 ID
    using ComponentType = std::uint8_t;                     // 컴포넌트 타입 ID
    using Signature = std::bitset<MAX_COMPONENTS>;          // 컴포넌트 조합 비트마스크
    
    // 특수 값들
    static constexpr Entity INVALID_ENTITY = 0;             // 유효하지 않은 엔티티
}
```

#### 1.2 Entity 관리 기본 구조
```cpp
// ECSEntity.h - Entity 관련 정의
#pragma once
#include "ECSConfig.h"
#include <queue>
#include <array>

namespace ECS {
    class EntityManager {
    private:
        std::queue<Entity> mAvailableEntities{};            // 재사용 가능한 ID
        std::array<Signature, MAX_ENTITIES> mSignatures{};  // 엔티티별 컴포넌트 조합
        Entity mLivingEntityCount = 0;                      // 현재 활성 엔티티 수
        Entity mNextEntityId = 1;                           // 다음 새 ID (0은 INVALID)

    public:
        Entity CreateEntity();
        void DestroyEntity(Entity entity);
        void SetSignature(Entity entity, Signature signature);
        Signature GetSignature(Entity entity) const;
        Entity GetLivingEntityCount() const;
    };
}
```

#### 1.3 기본 인터페이스 정의
```cpp
// 컴포넌트 배열 인터페이스
class IComponentArray {
public:
    virtual ~IComponentArray() = default;
    virtual void RemoveData(size_t index) = 0;
    virtual void MoveData(size_t sourceIndex, IComponentArray* destinationArray) = 0;
    // ... 기타 공통 인터페이스
};

// 시스템 인터페이스
class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void Update() = 0;
    virtual void LateUpdate() {}
    virtual void FixedUpdate() {}
    
protected:
    std::set<Entity> mEntities;  // 이 시스템이 처리하는 엔티티들
};
```

### ✅ **완료 검증**
- [ ] 컴파일 에러 없이 빌드 성공
- [ ] 기본 타입들이 올바른 크기인지 확인
- [ ] MAX_ENTITIES, MAX_COMPONENTS 설정이 요구사항에 맞는지 확인

### 📖 **다음 단계 준비**
- EntityManager의 실제 구현
- Entity 생성/삭제 로직
- Signature 관리 시스템

---

## Step 2: Entity 관리 시스템

### 📝 **이 단계에서 구현할 것**
- [ ] EntityManager 완전 구현
- [ ] Entity 생성/삭제 로직
- [ ] Signature 관리 시스템
- [ ] Entity 생명주기 관리

### 🎯 **목표**
Entity의 **생성, 삭제, 상태 관리**를 담당하는 시스템을 완성합니다.

### 📋 **구현 상세**

#### 2.1 EntityManager 완전 구현
```cpp
// ECSEntity.cpp
namespace ECS {
    Entity EntityManager::CreateEntity() {
        Entity id;
        
        if (!mAvailableEntities.empty()) {
            // 재사용 가능한 ID가 있으면 사용
            id = mAvailableEntities.front();
            mAvailableEntities.pop();
        } else {
            // 새 ID 생성
            if (mNextEntityId >= MAX_ENTITIES) {
                throw std::runtime_error("Maximum entity count exceeded");
            }
            id = mNextEntityId++;
        }
        
        ++mLivingEntityCount;
        mSignatures[id].reset();  // 시그니처 초기화
        return id;
    }
    
    void EntityManager::DestroyEntity(Entity entity) {
        if (entity >= MAX_ENTITIES || entity == INVALID_ENTITY) {
            throw std::out_of_range("Entity out of range");
        }
        
        mSignatures[entity].reset();
        mAvailableEntities.push(entity);
        --mLivingEntityCount;
    }
    
    void EntityManager::SetSignature(Entity entity, Signature signature) {
        if (entity >= MAX_ENTITIES || entity == INVALID_ENTITY) {
            throw std::out_of_range("Entity out of range");
        }
        mSignatures[entity] = signature;
    }
    
    Signature EntityManager::GetSignature(Entity entity) const {
        if (entity >= MAX_ENTITIES || entity == INVALID_ENTITY) {
            throw std::out_of_range("Entity out of range");
        }
        return mSignatures[entity];
    }
}
```

#### 2.2 Entity 생명주기 추적
```cpp
// 추가 기능: Entity 상태 추적
class EntityManager {
private:
    std::array<bool, MAX_ENTITIES> mIsAlive{};  // Entity 활성 상태

public:
    bool IsEntityAlive(Entity entity) const {
        return entity < MAX_ENTITIES && 
               entity != INVALID_ENTITY && 
               mIsAlive[entity];
    }
    
    // 디버깅용: 모든 활성 Entity 나열
    std::vector<Entity> GetAllLivingEntities() const {
        std::vector<Entity> result;
        for (Entity i = 1; i < mNextEntityId; ++i) {
            if (mIsAlive[i]) {
                result.push_back(i);
            }
        }
        return result;
    }
};
```

### ✅ **테스트 시나리오**
```cpp
// EntityManager 기본 테스트
void TestEntityManager() {
    EntityManager manager;
    
    // 1. Entity 생성 테스트
    Entity e1 = manager.CreateEntity();
    Entity e2 = manager.CreateEntity();
    assert(e1 != e2);
    assert(manager.GetLivingEntityCount() == 2);
    
    // 2. Signature 설정 테스트
    Signature sig;
    sig.set(0);  // 첫 번째 컴포넌트 타입
    manager.SetSignature(e1, sig);
    assert(manager.GetSignature(e1) == sig);
    
    // 3. Entity 삭제 및 재사용 테스트
    manager.DestroyEntity(e1);
    assert(manager.GetLivingEntityCount() == 1);
    
    Entity e3 = manager.CreateEntity();
    assert(e3 == e1);  // ID 재사용 확인
}
```

### 📖 **다음 단계 준비**
- Component 타입 등록 시스템
- ComponentArray 템플릿 구현
- 타입 안전성 확보

---

## Step 3: Component 시스템

### 📝 **이 단계에서 구현할 것**
- [ ] ComponentArray 템플릿 구현
- [ ] Component 타입 등록 시스템
- [ ] 타입 안전한 Component 관리
- [ ] ComponentManager 구현

### 🎯 **목표**
**타입 안전하고 성능 최적화된** Component 저장 및 관리 시스템을 구축합니다.

### 📋 **구현 상세**

#### 3.1 ComponentArray 템플릿 구현
```cpp
// ECSComponent.h
namespace ECS {
    template<typename T>
    class ComponentArray : public IComponentArray {
    private:
        std::vector<T> mComponentArray;
        std::unordered_map<Entity, size_t> mEntityToIndexMap;
        std::unordered_map<size_t, Entity> mIndexToEntityMap;
        size_t mSize = 0;

    public:
        void InsertData(Entity entity, T component) {
            if (mEntityToIndexMap.count(entity)) {
                throw std::runtime_error("Component added to same entity more than once");
            }
            
            size_t newIndex = mSize;
            mEntityToIndexMap[entity] = newIndex;
            mIndexToEntityMap[newIndex] = entity;
            mComponentArray.push_back(component);
            ++mSize;
        }
        
        void RemoveData(Entity entity) {
            if (!mEntityToIndexMap.count(entity)) {
                throw std::runtime_error("Removing non-existent component");
            }
            
            // Swap-and-pop 최적화
            size_t indexOfRemovedEntity = mEntityToIndexMap[entity];
            size_t indexOfLastElement = mSize - 1;
            
            // 마지막 요소를 제거할 위치로 이동
            mComponentArray[indexOfRemovedEntity] = mComponentArray[indexOfLastElement];
            
            // 맵 업데이트
            Entity entityOfLastElement = mIndexToEntityMap[indexOfLastElement];
            mEntityToIndexMap[entityOfLastElement] = indexOfRemovedEntity;
            mIndexToEntityMap[indexOfRemovedEntity] = entityOfLastElement;
            
            // 제거된 엔티티 정보 삭제
            mEntityToIndexMap.erase(entity);
            mIndexToEntityMap.erase(indexOfLastElement);
            
            mComponentArray.pop_back();
            --mSize;
        }
        
        T& GetData(Entity entity) {
            if (!mEntityToIndexMap.count(entity)) {
                throw std::runtime_error("Retrieving non-existent component");
            }
            return mComponentArray[mEntityToIndexMap[entity]];
        }
        
        void EntityDestroyed(Entity entity) override {
            if (mEntityToIndexMap.count(entity)) {
                RemoveData(entity);
            }
        }
    };
}
```

#### 3.2 ComponentManager 구현
```cpp
namespace ECS {
    class ComponentManager {
    private:
        std::unordered_map<std::type_index, ComponentType> mComponentTypes;
        std::unordered_map<std::type_index, std::unique_ptr<IComponentArray>> mComponentArrays;
        ComponentType mNextComponentType = 0;

    public:
        template<typename T>
        void RegisterComponent() {
            std::type_index type = std::type_index(typeid(T));
            
            if (mComponentTypes.count(type)) {
                throw std::runtime_error("Component type already registered");
            }
            
            mComponentTypes[type] = mNextComponentType;
            mComponentArrays[type] = std::make_unique<ComponentArray<T>>();
            ++mNextComponentType;
        }
        
        template<typename T>
        ComponentType GetComponentType() {
            std::type_index type = std::type_index(typeid(T));
            
            if (!mComponentTypes.count(type)) {
                throw std::runtime_error("Component not registered before use");
            }
            
            return mComponentTypes[type];
        }
        
        template<typename T>
        void AddComponent(Entity entity, T component) {
            GetComponentArray<T>()->InsertData(entity, component);
        }
        
        template<typename T>
        void RemoveComponent(Entity entity) {
            GetComponentArray<T>()->RemoveData(entity);
        }
        
        template<typename T>
        T& GetComponent(Entity entity) {
            return GetComponentArray<T>()->GetData(entity);
        }
        
        void EntityDestroyed(Entity entity) {
            for (auto const& pair : mComponentArrays) {
                pair.second->EntityDestroyed(entity);
            }
        }

    private:
        template<typename T>
        ComponentArray<T>* GetComponentArray() {
            std::type_index type = std::type_index(typeid(T));
            
            if (!mComponentTypes.count(type)) {
                throw std::runtime_error("Component not registered before use");
            }
            
            return static_cast<ComponentArray<T>*>(mComponentArrays[type].get());
        }
    };
}
```

#### 3.3 컴포넌트 사용 예제
```cpp
// 예제 컴포넌트들
struct TransformComponent {
    static const char* GetName() { return "TransformComponent"; }
    float x, y, z;
    float rotX, rotY, rotZ;
    float scaleX = 1.0f, scaleY = 1.0f, scaleZ = 1.0f;
};

struct VelocityComponent {
    static const char* GetName() { return "VelocityComponent"; }
    float dx, dy, dz;
};

// 사용법
void ExampleUsage() {
    ComponentManager componentManager;
    EntityManager entityManager;
    
    // 1. 컴포넌트 타입 등록
    componentManager.RegisterComponent<TransformComponent>();
    componentManager.RegisterComponent<VelocityComponent>();
    
    // 2. 엔티티 생성 및 컴포넌트 추가
    Entity entity = entityManager.CreateEntity();
    
    componentManager.AddComponent<TransformComponent>(entity, {0.0f, 0.0f, 0.0f});
    componentManager.AddComponent<VelocityComponent>(entity, {1.0f, 0.0f, 0.0f});
    
    // 3. 엔티티 시그니처 설정
    Signature signature;
    signature.set(componentManager.GetComponentType<TransformComponent>());
    signature.set(componentManager.GetComponentType<VelocityComponent>());
    entityManager.SetSignature(entity, signature);
    
    // 4. 컴포넌트 사용
    auto& transform = componentManager.GetComponent<TransformComponent>(entity);
    auto& velocity = componentManager.GetComponent<VelocityComponent>(entity);
    
    transform.x += velocity.dx;  // 물리 업데이트 예제
}
```

### ✅ **테스트 시나리오**
```cpp
void TestComponentSystem() {
    ComponentManager cm;
    EntityManager em;
    
    // 1. 컴포넌트 등록 테스트
    cm.RegisterComponent<TransformComponent>();
    cm.RegisterComponent<VelocityComponent>();
    
    // 2. 컴포넌트 추가/제거 테스트
    Entity e1 = em.CreateEntity();
    cm.AddComponent<TransformComponent>(e1, {1.0f, 2.0f, 3.0f});
    
    auto& transform = cm.GetComponent<TransformComponent>(e1);
    assert(transform.x == 1.0f);
    
    // 3. Swap-and-pop 최적화 테스트
    Entity e2 = em.CreateEntity();
    cm.AddComponent<TransformComponent>(e2, {4.0f, 5.0f, 6.0f});
    
    cm.RemoveComponent<TransformComponent>(e1);  // 첫 번째 제거
    
    // e2의 데이터가 올바른 위치에 있는지 확인
    auto& transform2 = cm.GetComponent<TransformComponent>(e2);
    assert(transform2.x == 4.0f);
}
```

### 📖 **다음 단계 준비**
- Archetype 시스템 설계
- 메모리 지역성 최적화
- Entity-Component 관계를 Archetype으로 재구성

---

## Step 4: Archetype 기반 메모리 관리

### 📝 **이 단계에서 구현할 것**
- [ ] Archetype 클래스 구현
- [ ] ArchetypeManager 구현
- [ ] Entity Migration 시스템
- [ ] 메모리 최적화된 Component 저장

### 🎯 **목표**
**메모리 지역성을 극대화**하고 **캐시 효율성을 향상**시키는 Archetype 시스템을 구현합니다.

### 📋 **구현 상세**

이 단계는 ECS의 핵심이므로 더 자세한 설명이 필요합니다. 

#### 4.1 Archetype 설계 원리
```cpp
// Archetype 개념 이해
/*
전통적인 방식:
Entity1: [Transform] -> 메모리 위치 A
Entity2: [Transform] -> 메모리 위치 B (멀리 떨어져 있음)
Entity3: [Transform] -> 메모리 위치 C (더 멀리 떨어져 있음)

Archetype 방식:
Archetype1 (Transform만 가진 엔티티들):
  Entities: [Entity1, Entity2, Entity3]
  Transform[]: [data1, data2, data3]  <- 연속된 메모리!
*/
```

### 🤔 **진행 방식 선택**

여기서 두 가지 방향으로 진행할 수 있습니다:

**A) 단순한 Archetype 구현 (권장)**
- 기본적인 Archetype 기능만 구현
- 이해하기 쉽고 디버깅 용이
- 실용적인 성능 향상

**B) 고급 Archetype 구현**
- Shared Component, Chunk 기반 메모리 관리
- 최대 성능 최적화
- 구현 복잡도 높음

어느 방향으로 진행하시겠습니까? 

또는 **Step 4를 더 세분화**해서:
- Step 4A: 기본 Archetype 구조
- Step 4B: Entity Migration
- Step 4C: 메모리 최적화

이렇게 나누는 것도 가능합니다.

### 📊 **현재 구현 현황 체크**

현재까지 완성된 부분:
- ✅ Step 1: 기본 구조 (ECSConfig, Entity 타입)
- ✅ Step 2: EntityManager 
- ⏳ Step 3: ComponentManager (진행 중)
- ⏸️ Step 4: Archetype 시스템 (대기 중)

**다음 진행 방향을 선택해주세요:**

1. **Step 3 완료 후 Step 4A (기본 Archetype)**
2. **Step 4를 더 세분화해서 진행**
3. **현재 코드 검토 후 맞춤형 가이드 제공**

어떤 방향으로 진행하시겠습니까?

---

## 📝 **임시 체크포인트**

### 현재 위치
- **완료된 설계**: Entity 관리, Component 기본 구조
- **진행 중**: Step 3 (Component 시스템)
- **다음 단계**: Archetype 구현 전략 결정

### 결정 필요 사항
1. Archetype 구현 복잡도 레벨
2. Step 세분화 정도
3. 현재 코드와의 통합 방식

**여러분의 선택에 따라 다음 단계를 구체화하겠습니다!** 🚀
