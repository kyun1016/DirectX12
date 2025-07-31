# ECS 게임 엔진 시스템 완전 가이드

## 목차
1. [시스템 개요](#시스템-개요)
2. [핵심 아키텍처](#핵심-아키텍처)
3. [시스템 생명주기](#시스템-생명주기)
4. [컴포넌트 시스템](#컴포넌트-시스템)
5. [엔티티 관리](#엔티티-관리)
6. [스레드 안전성](#스레드-안전성)
7. [DirectX12 통합](#directx12-통합)
8. [실제 사용 예제](#실제-사용-예제)

> 📋 **상세 시스템 가이드**: 각 시스템의 세부 기능과 사용법은 **[SYSTEMS_GUIDE.md](./SYSTEMS_GUIDE.md)**를 참조하세요.

---

## 시스템 개요

이 프로젝트는 **Entity-Component-System (ECS)** 아키텍처와 **DirectX12**를 결합한 고성능 게임 엔진입니다.

### 주요 특징
- **Archetype 기반** 컴포넌트 저장으로 메모리 효율성 극대화
- **멀티스레드** 시스템 업데이트 지원
- **타입 안전성**을 보장하는 템플릿 기반 설계
- **DirectX12** 모던 그래픽스 API 완전 통합
- **JSON 기반** 월드 직렬화/역직렬화

### 기술 스택
```cpp
- 언어: C++20
- 그래픽스: DirectX12
- 오디오: FMOD
- UI: ImGui
- JSON: nlohmann/json
- 수학: SimpleMath
```

---

## 핵심 아키텍처

### 1. ECS 트리아드 구조

```
┌─────────────────┐
│   Coordinator   │ ← 싱글톤 중앙 관리자
│                 │
├─────────────────┤
│ EntityManager   │ ← 엔티티 생성/삭제 관리
│ ArchetypeManager│ ← 컴포넌트 저장 최적화
│ SystemManager   │ ← 시스템 생명주기 관리
│SingletonCompMgr │ ← 전역 컴포넌트 관리
└─────────────────┘
```

### 2. 시스템 아키텍처 계층

```
Application Layer   │ main.cpp, Game Logic
────────────────────┼─────────────────────
ECS Core Layer      │ Coordinator, Managers
────────────────────┼─────────────────────
Component Layer     │ Components, Archetypes
────────────────────┼─────────────────────
System Layer        │ Update Systems
────────────────────┼─────────────────────
Platform Layer      │ DirectX12, FMOD, Win32
```

---

## 시스템 생명주기

### 완전한 업데이트 사이클

시스템은 다음과 같은 **8단계 생명주기**를 따릅니다:

```cpp
1. BeginPlay     │ 시스템 초기화 (게임 시작시 1회)
2. Sync          │ 윈도우/입력 동기화 (메인 스레드 전용)
3. PreUpdate     │ 업데이트 전 준비
4. Update        │ 핵심 게임 로직
5. LateUpdate    │ 후처리 업데이트
6. FixedUpdate   │ 물리 연산 등 고정 업데이트
7. FinalUpdate   │ 렌더링 준비
8. EndPlay       │ 시스템 정리 (게임 종료시 1회)
```

### 게임 루프 실행 순서

```cpp
// ECSCoordinator.cpp - Run() 메서드
void Coordinator::Run()
{
    mSystemManager->BeginPlayAllSystems();           // 1. 초기화
    
    while (gameRunning) 
    {
        // 메인 스레드 전용 (WinProc 충돌 방지)
        if (!WindowSystem::GetInstance().Sync())     // 2. 윈도우 동기화
            break;
        
        InputSystem::GetInstance().PreUpdate();      // 입력 전처리
        
        // 병렬 처리 가능한 시스템들
        mSystemManager->SyncAllSystems();            // 3. 시스템 동기화
        mSystemManager->PreUpdateAllSystems();       // 4. 전처리
        mSystemManager->UpdateAllSystems();          // 5. 메인 업데이트
        
        ImGuiSystem::GetInstance().RenderMultiViewport(); // UI 렌더링
        
        mSystemManager->LateUpdateAllSystems();      // 6. 후처리
        mSystemManager->FixedUpdateAllSystems();     // 7. 고정 업데이트
        mSystemManager->FinalUpdateAllSystems();     // 8. 최종 업데이트
    }
    
    mSystemManager->EndPlayAllSystems();             // 9. 정리
}
```

### 멀티스레드 처리 방식

```cpp
// 각 업데이트 단계는 병렬로 실행됩니다
inline void UpdateAllSystems() {
    std::vector<std::future<void>> futures;
    
    // 모든 시스템을 비동기로 시작
    for (auto& [_, task] : mSystemUpdateTasks)
        futures.emplace_back(std::async(std::launch::async, task));
    
    // 모든 작업 완료 대기 (순차적)
    for (auto& fut : futures)
        fut.get();
}
```

---

## 컴포넌트 시스템

### 1. 컴포넌트 정의 및 등록

```cpp
// 컴포넌트 구조체 예제
struct TransformComponent {
    Vector3 position{0.0f, 0.0f, 0.0f};
    Vector3 rotation{0.0f, 0.0f, 0.0f};
    Vector3 scale{1.0f, 1.0f, 1.0f};
    Matrix4x4 worldMatrix;
};

// 컴포넌트 등록
void RegisterComponents() {
    gCoordinator.RegisterComponent<TransformComponent>();
    gCoordinator.RegisterComponent<MeshComponent>();
    gCoordinator.RegisterComponent<MaterialComponent>();
    // ... 기타 컴포넌트들
}
```

### 2. Archetype 기반 저장 시스템

```cpp
// ECSArchetype.h - 메모리 효율적인 컴포넌트 저장
template<typename T>
class ComponentArray : public IComponentArray {
private:
    std::array<T, MAX_ENTITIES> mComponentArray;      // 연속된 메모리
    std::unordered_map<Entity, size_t> mEntityToIndexMap;
    std::unordered_map<size_t, Entity> mIndexToEntityMap;
    size_t mSize;
    
public:
    void InsertData(Entity entity, T component) {
        size_t newIndex = mSize;
        mEntityToIndexMap[entity] = newIndex;
        mIndexToEntityMap[newIndex] = entity;
        mComponentArray[newIndex] = component;
        ++mSize;
    }
    
    T& GetData(Entity entity) {
        return mComponentArray[mEntityToIndexMap[entity]];
    }
};
```

### 3. 컴포넌트 시그니처 시스템

```cpp
// 비트마스크를 이용한 효율적인 엔티티 필터링
using Signature = std::bitset<MAX_COMPONENTS>;  // 32비트 최대

// 시스템별 시그니처 설정
void SetupSystems() {
    // 렌더링 시스템: Transform + Mesh + Material 필요
    Signature renderSignature;
    renderSignature.set(gCoordinator.GetComponentType<TransformComponent>());
    renderSignature.set(gCoordinator.GetComponentType<MeshComponent>());
    renderSignature.set(gCoordinator.GetComponentType<MaterialComponent>());
    
    gCoordinator.SetSystemSignature<RenderSystem>(renderSignature);
}
```

---

## 엔티티 관리

### 1. 엔티티 생명주기

```cpp
// 엔티티 생성
Entity entity = gCoordinator.CreateEntity();

// 컴포넌트 추가
gCoordinator.AddComponent<TransformComponent>(entity, {
    .position = {0.0f, 0.0f, 0.0f},
    .rotation = {0.0f, 0.0f, 0.0f},
    .scale = {1.0f, 1.0f, 1.0f}
});

gCoordinator.AddComponent<MeshComponent>(entity, {
    .meshHandle = meshRepository.LoadMesh("cube.fbx")
});

// 컴포넌트 접근
auto& transform = gCoordinator.GetComponent<TransformComponent>(entity);
transform.position.x += deltaTime * speed;

// 엔티티 삭제
gCoordinator.DestroyEntity(entity);
```

### 2. 동적 시그니처 업데이트

```cpp
// 엔티티의 시그니처가 변경될 때 자동으로 시스템 필터링 업데이트
void EntitySignatureChanged(Entity entity, Signature entitySignature) {
    for (auto const& pair : mSystems) {
        auto const& systemSignature = mSignatures[pair.first];
        
        // 비트마스크 AND 연산으로 빠른 매칭
        if ((entitySignature & systemSignature) == systemSignature) {
            pair.second->mEntities.insert(entity);  // 시스템에 추가
        } else {
            pair.second->mEntities.erase(entity);   // 시스템에서 제거
        }
    }
}
```

---

## 스레드 안전성

### 1. 뮤텍스 기반 동기화

```cpp
// ECSCoordinator.h - 스레드 안전한 중앙 관리
class Coordinator {
private:
    mutable std::mutex mMutex;  // 모든 작업에 대한 보호
    
public:
    template<typename T>
    T& GetComponent(Entity entity) {
        std::lock_guard<std::mutex> lock(mMutex);
        return mArchetypeManager->GetComponent<T>(entity);
    }
    
    template<typename T>
    void AddComponent(Entity entity, T component) {
        std::lock_guard<std::mutex> lock(mMutex);
        mArchetypeManager->AddComponent<T>(entity, component);
        
        auto signature = mEntityManager->GetSignature(entity);
        signature.set(mComponentManager->GetComponentType<T>());
        mEntityManager->SetSignature(entity, signature);
        
        mSystemManager->EntitySignatureChanged(entity, signature);
    }
};
```

### 2. 예외 처리 고려사항

```cpp
// 현재는 주석 처리되어 있지만, 예외 안전성을 위한 구조
inline void UpdateAllSystems() {
    std::vector<std::future<void>> futures;
    for (auto& [_, task] : mSystemUpdateTasks)
        futures.emplace_back(std::async(std::launch::async, task));
    
    for (auto& fut : futures) {
        // TODO: 예외 처리 활성화 예정
        // try {
        //     fut.get();
        // } catch (const std::exception& e) {
        //     LOG_ERROR("Exception in system task: {}", e.what());
        // }
        fut.get();
    }
}
```

---

## DirectX12 통합

### 1. 렌더링 파이프라인 시스템

```cpp
// 주요 DirectX12 시스템들
DX12_DeviceSystem        │ 디바이스 및 큐 관리
DX12_SwapChainSystem     │ 스왑체인 및 백버퍼 관리
DX12_CommandSystem       │ 커맨드 리스트 관리
DX12_FrameResourceSystem │ 프레임별 리소스 관리
DX12_RenderSystem        │ 실제 렌더링 수행
DX12_PSOSystem          │ 파이프라인 상태 관리
```

> 💡 **상세 정보**: 각 DirectX12 시스템의 구체적인 기능과 API는 **[SYSTEMS_GUIDE.md](./SYSTEMS_GUIDE.md#directx12-렌더링-시스템)**에서 확인하세요.

### 2. 리소스 관리 시스템

```cpp
// Repository 패턴을 통한 효율적인 리소스 관리
DX12_MeshRepository      │ 메시 데이터 캐싱
DX12_RootSignatureRepository │ 루트 시그니처 재사용
DX12_RTVHeapRepository   │ 렌더 타겟 뷰 힙 관리
DX12_DSVHeapRepository   │ 깊이 스텐실 뷰 힙 관리
```

### 3. 컴포넌트 기반 렌더링

```cpp
// 렌더링 관련 컴포넌트들
struct DX12_RenderComponent {
    bool isVisible = true;
    uint32_t renderQueue = 0;  // 렌더링 순서
};

struct DX12_MeshComponent {
    RepoHandle meshHandle;      // MeshRepository 핸들
    BoundingBox boundingBox;    // 컬링용 바운딩 박스
};

struct MaterialComponent {
    RepoHandle albedoTexture;
    RepoHandle normalTexture;
    RepoHandle metallicTexture;
    Vector4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 1.0f;
};
```

---

## 실제 사용 예제

### 1. 게임 오브젝트 생성 예제

```cpp
// main.cpp에서 사용되는 실제 예제
void CreateRenderableEntity() {
    Entity entity = gCoordinator.CreateEntity();
    
    // Transform 컴포넌트
    gCoordinator.AddComponent<TransformComponent>(entity, {
        .position = {0.0f, 0.0f, 0.0f},
        .rotation = {0.0f, 0.0f, 0.0f},
        .scale = {1.0f, 1.0f, 1.0f}
    });
    
    // 메시 컴포넌트
    auto meshHandle = MeshRepository::GetInstance().LoadMesh("Models/cube.fbx");
    gCoordinator.AddComponent<DX12_MeshComponent>(entity, {
        .meshHandle = meshHandle
    });
    
    // 머티리얼 컴포넌트
    gCoordinator.AddComponent<MaterialComponent>(entity, {
        .albedoTexture = TextureSystem::LoadTexture("Textures/default.png"),
        .baseColor = {1.0f, 0.5f, 0.2f, 1.0f},
        .metallic = 0.0f,
        .roughness = 0.8f
    });
    
    // 렌더링 컴포넌트
    gCoordinator.AddComponent<DX12_RenderComponent>(entity, {
        .isVisible = true,
        .renderQueue = 0
    });
}
```

### 2. 물리 시뮬레이션 예제

```cpp
void InitPhysicsExample() {
    for (int i = 0; i < 100; ++i) {
        Entity entity = gCoordinator.CreateEntity();
        
        // 위치 컴포넌트
        gCoordinator.AddComponent<TransformComponent>(entity, {
            .position = {
                static_cast<float>(rand() % 10),
                static_cast<float>(rand() % 10),
                static_cast<float>(rand() % 10)
            }
        });
        
        // 강체 컴포넌트
        gCoordinator.AddComponent<RigidBodyComponent>(entity, {
            .velocity = {0.0f, 0.0f, 0.0f},
            .mass = 1.0f,
            .drag = 0.1f
        });
        
        // 중력 컴포넌트
        gCoordinator.AddComponent<GravityComponent>(entity, {
            .force = {0.0f, -9.81f, 0.0f}
        });
    }
}
```

### 3. 시스템 등록 및 시그니처 설정

```cpp
void SetupECSWorld() {
    // 1. 컴포넌트 등록
    RegisterAllComponents();
    
    // 2. 시스템 등록
    auto renderSystem = gCoordinator.RegisterSystem<DX12_RenderSystem>();
    auto physicsSystem = gCoordinator.RegisterSystem<PhysicsSystem>();
    auto transformSystem = gCoordinator.RegisterSystem<WorldMatrixUpdateSystem>();
    
    // 3. 시스템 시그니처 설정
    Signature renderSignature;
    renderSignature.set(gCoordinator.GetComponentType<TransformComponent>());
    renderSignature.set(gCoordinator.GetComponentType<DX12_MeshComponent>());
    renderSignature.set(gCoordinator.GetComponentType<DX12_RenderComponent>());
    gCoordinator.SetSystemSignature<DX12_RenderSystem>(renderSignature);
    
    Signature physicsSignature;
    physicsSignature.set(gCoordinator.GetComponentType<TransformComponent>());
    physicsSignature.set(gCoordinator.GetComponentType<RigidBodyComponent>());
    gCoordinator.SetSystemSignature<PhysicsSystem>(physicsSignature);
}
```

### 4. JSON 기반 월드 직렬화

```cpp
// world.json 형태로 월드 상태 저장/로드
{
    "entities": [
        {
            "id": 1,
            "components": {
                "TransformComponent": {
                    "position": [0.0, 0.0, 0.0],
                    "rotation": [0.0, 0.0, 0.0],
                    "scale": [1.0, 1.0, 1.0]
                },
                "MeshComponent": {
                    "meshPath": "Models/cube.fbx"
                }
            }
        }
    ],
    "systems": {
        "RenderSystem": {
            "enabled": true,
            "priority": 0
        }
    }
}
```

---

## 성능 및 최적화

### 1. 메모리 최적화
- **Archetype 기반 저장**: 같은 컴포넌트 조합을 가진 엔티티들을 연속된 메모리에 저장
- **Pool 할당**: 고정 크기 배열(`MAX_ENTITIES = 5000`)로 동적 할당 최소화
- **비트마스크 시그니처**: 32비트 비트셋으로 빠른 컴포넌트 매칭

### 2. 병렬 처리 최적화
- **시스템별 독립 실행**: 각 업데이트 단계에서 시스템들이 병렬로 실행
- **스레드 안전성**: 뮤텍스로 데이터 레이스 방지
- **예외 분리**: WinProc만 메인 스레드에서 처리

### 3. 렌더링 최적화
- **Repository 패턴**: 리소스 중복 로딩 방지
- **컬링 시스템**: BoundingBox 기반 프러스텀 컬링
- **배치 렌더링**: 동일한 머티리얼/메시 그룹핑

---

## 확장 가능성

### 1. 새로운 시스템 추가
```cpp
class CustomSystem : public ECS::ISystem {
public:
    void Update() override {
        for (auto const& entity : mEntities) {
            // 커스텀 로직
        }
    }
};

// 등록
auto customSystem = gCoordinator.RegisterSystem<CustomSystem>();
Signature customSignature;
customSignature.set(gCoordinator.GetComponentType<CustomComponent>());
gCoordinator.SetSystemSignature<CustomSystem>(customSignature);
```

### 2. 플랫폼 확장
- **DirectX11**: `DX11_Config.h` 준비됨
- **Vulkan**: `Vulkan_Config.h` 계획됨
- **크로스 플랫폼**: 추상화 레이어 통해 확장 가능

### 3. 네트워킹 통합
- ECS 구조상 네트워크 동기화 용이
- 컴포넌트 단위 직렬화로 대역폭 최적화 가능

---

## 문서 구조

- **[README.md](./README.md)** (현재 문서): ECS 시스템 전체 개요 및 아키텍처
- **[SYSTEMS_GUIDE.md](./SYSTEMS_GUIDE.md)**: 개별 시스템별 상세 기능 가이드
- **[COMPONENTS_GUIDE.md](./COMPONENTS_GUIDE.md)**: 모든 컴포넌트의 구조와 사용법

---

## 결론

이 ECS 시스템은 다음과 같은 강점을 가집니다:

1. **높은 성능**: Archetype + 멀티스레딩으로 대량의 엔티티 처리 가능
2. **확장성**: 모듈화된 시스템 구조로 새로운 기능 추가 용이
3. **안정성**: 타입 안전성과 스레드 안전성 보장
4. **현대적**: DirectX12, C++20 등 최신 기술 스택 활용

현재 8.2/10의 완성도를 가지고 있으며, 로깅 시스템과 예외 처리 개선을 통해 더욱 견고한 엔진으로 발전시킬 수 있습니다.
