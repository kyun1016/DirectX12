# ECS 컴포넌트 상세 가이드

## 목차
1. [컴포넌트 개요](#컴포넌트-개요)
2. [Archetype 시스템 심화](#archetype-시스템-심화)
3. [코어 컴포넌트](#코어-컴포넌트)
4. [렌더링 컴포넌트](#렌더링-컴포넌트)
5. [물리 컴포넌트](#물리-컴포넌트)
6. [게임플레이 컴포넌트](#게임플레이-컴포넌트)
7. [오디오 컴포넌트](#오디오-컴포넌트)
8. [UI/입력 컴포넌트](#ui입력-컴포넌트)
9. [시스템별 컴포넌트 조합](#시스템별-컴포넌트-조합)
10. [커스텀 컴포넌트 생성 가이드](#커스텀-컴포넌트-생성-가이드)

---

## 컴포넌트 개요

이 ECS 시스템은 **데이터 중심 설계**로 컴포넌트들을 순수한 데이터 구조체로 정의합니다.

### 컴포넌트 설계 원칙

1. **데이터만 포함**: 로직은 시스템에서 처리
2. **JSON 직렬화 지원**: 월드 저장/로드용
3. **메모리 효율성**: POD(Plain Old Data) 구조 선호
4. **타입 안전성**: 템플릿 기반 강타입 시스템

### 기본 컴포넌트 구조

```cpp
struct ExampleComponent {
    static const char* GetName() { return "ExampleComponent"; }
    
    // 실제 데이터 멤버들
    float value;
    Vector3 position;
    bool isActive = true;
};

// JSON 직렬화 지원
inline void to_json(json& j, const ExampleComponent& p) {
    j = json{
        {"value", p.value},
        {"position", {p.position.x, p.position.y, p.position.z}},
        {"isActive", p.isActive}
    };
}

inline void from_json(const json& j, ExampleComponent& p) {
    j.at("value").get_to(p.value);
    auto pos = j.at("position");
    p.position.x = pos[0]; p.position.y = pos[1]; p.position.z = pos[2];
    p.isActive = j.at("isActive").get<bool>();
}
```

---

## Archetype 시스템 심화

### Archetype란?

**Archetype**은 동일한 컴포넌트 조합을 가진 엔티티들을 하나의 그룹으로 관리하는 ECS 최적화 기법입니다.

#### 핵심 개념
```cpp
// 예시: Transform + Mesh + Material 컴포넌트를 가진 엔티티들
Archetype "RenderableObject" {
    Signature: [Transform=1, Mesh=1, Material=1]  // 비트마스크
    Entities: [Entity1, Entity5, Entity12, ...]   // 같은 구조의 엔티티들
    ComponentArrays: {
        TransformComponent[]: [data1, data5, data12, ...]
        MeshComponent[]:      [data1, data5, data12, ...]
        MaterialComponent[]:  [data1, data5, data12, ...]
    }
}
```

### Archetype의 장점

#### 1. **메모리 지역성 (Cache Locality)**
```cpp
// ❌ 전통적인 방식: 엔티티별 컴포넌트 분산
Entity1 -> [Transform, Mesh, Material] (메모리 주소 A)
Entity2 -> [Transform, Physics]        (메모리 주소 B)
Entity3 -> [Transform, Mesh, Material] (메모리 주소 C)

// ✅ Archetype 방식: 컴포넌트별 연속 배열
Archetype1 (Transform+Mesh+Material):
  Transform[]: [Entity1_Transform, Entity3_Transform, ...]  // 연속 메모리
  Mesh[]:      [Entity1_Mesh, Entity3_Mesh, ...]           // 연속 메모리
  Material[]:  [Entity1_Material, Entity3_Material, ...]   // 연속 메모리
```

#### 2. **시스템 처리 효율성**
```cpp
// 렌더링 시스템에서의 처리
class RenderSystem : public ECS::ISystem {
public:
    void Update() override {
        // 렌더링에 필요한 Archetype만 선택적으로 처리
        for (auto* archetype : GetArchetypesWithSignature(renderSignature)) {
            auto* transforms = archetype->GetComponentArray<TransformComponent>();
            auto* meshes = archetype->GetComponentArray<MeshComponent>();
            auto* materials = archetype->GetComponentArray<MaterialComponent>();
            
            // 벡터화 최적화 가능: 동일한 타입의 데이터가 연속 배열로 저장됨
            for (size_t i = 0; i < archetype->GetEntityCount(); ++i) {
                ProcessRenderItem(transforms[i], meshes[i], materials[i]);
            }
        }
    }
};
```

### ComponentArray 클래스

#### 동적 크기 관리
```cpp
template<typename T>
class ComponentArray : public IComponentArray {
private:
    std::vector<T> mComponentArray;  // 동적 배열

public:
    ComponentArray() {
        mComponentArray.reserve(64);  // 기본 64개 예약
    }
    
    // 성능 최적화: 미리 용량 예약
    void Reserve(size_t capacity) {
        mComponentArray.reserve(capacity);
    }
    
    // 데이터 추가 (O(1) amortized)
    void InsertData(T component) {
        mComponentArray.push_back(component);
    }
    
    // 데이터 접근 (O(1))
    T& GetData(size_t index) {
        if (index >= mComponentArray.size()) {
            throw std::out_of_range("Component array index out of bounds");
        }
        return mComponentArray[index];
    }
};
```

#### Swap-and-Pop 최적화
```cpp
// 엔티티 제거 시 배열 밀집도 유지
void RemoveData(size_t index) override {
    if (index >= mComponentArray.size()) {
        throw std::out_of_range("Cannot remove component");
    }
    
    // 제거할 요소와 마지막 요소를 교환
    mComponentArray[index] = std::move(mComponentArray.back());
    
    // 마지막 요소 제거 (O(1))
    mComponentArray.pop_back();
    
    // 결과: 배열에 구멍이 생기지 않고 밀집도 유지
}
```

### Archetype 클래스

#### 구조와 역할
```cpp
class Archetype {
private:
    Signature mSignature;                    // 컴포넌트 조합 비트마스크
    size_t mSharedComponentId;               // 공유 컴포넌트 ID
    std::vector<Entity> mEntities;           // 이 Archetype에 속한 엔티티들
    std::unordered_map<Entity, size_t> mEntityToIndexMap;  // 엔티티→인덱스 매핑
    
    // 컴포넌트별 배열들
    std::unordered_map<ComponentType, std::unique_ptr<IComponentArray>> mComponentArrays;

public:
    // 엔티티 추가 (O(1))
    size_t AddEntity(Entity entity) {
        size_t newIndex = mEntities.size();
        mEntityToIndexMap[entity] = newIndex;
        mEntities.push_back(entity);
        return newIndex;
    }
    
    // 엔티티 제거 (O(1) with swap-and-pop)
    Entity RemoveEntity(Entity entity) {
        size_t indexOfRemoved = mEntityToIndexMap[entity];
        size_t indexOfLast = mEntities.size() - 1;
        
        // 모든 컴포넌트 배열에서 swap-and-pop
        for (auto& [type, array] : mComponentArrays) {
            array->RemoveData(indexOfRemoved);
        }
        
        // 엔티티 배열에서도 swap-and-pop
        mEntities[indexOfRemoved] = mEntities[indexOfLast];
        mEntities.pop_back();
        
        // 맵 업데이트
        mEntityToIndexMap.erase(entity);
        if (indexOfLast != indexOfRemoved) {
            Entity movedEntity = mEntities[indexOfRemoved];
            mEntityToIndexMap[movedEntity] = indexOfRemoved;
            return movedEntity;  // 이동된 엔티티 반환
        }
        return INVALID_ENTITY;
    }
};
```

### ArchetypeManager 클래스

#### Archetype 관리 전략
```cpp
class ArchetypeManager {
private:
    // 시그니처별 → 공유컴포넌트ID별 → Archetype 매핑
    std::unordered_map<Signature, 
                       std::unordered_map<size_t, std::unique_ptr<Archetype>>> mArchetypes;
    
    // 엔티티 위치 추적
    std::unordered_map<Entity, EntityLocation> mEntityLocations;

public:
    // Archetype 찾기 또는 생성
    Archetype* FindOrCreateArchetype(const Signature& signature, size_t sharedId) {
        if (mArchetypes[signature].find(sharedId) == mArchetypes[signature].end()) {
            // 새 Archetype 생성
            mArchetypes[signature][sharedId] = 
                std::make_unique<Archetype>(signature, sharedId, this);
        }
        return mArchetypes[signature][sharedId].get();
    }
};
```

### 엔티티 이동 (Migration) 시스템

#### 컴포넌트 추가 시 Archetype 이동
```cpp
template<typename T>
void AddComponent(Entity entity, const T& component, const Signature& oldSignature) {
    // 1. 현재 위치 확인
    EntityLocation oldLocation = mEntityLocations.count(entity) ? 
                                mEntityLocations[entity] : EntityLocation{nullptr, 0};
    
    // 2. 새로운 시그니처 계산
    Signature newSignature = oldSignature;
    newSignature.set(GetComponentType<T>());
    
    // 3. 목적지 Archetype 찾기/생성
    size_t sharedId = oldLocation.archetype ? 
                      oldLocation.archetype->GetSharedComponentId() : 0;
    Archetype* newArchetype = FindOrCreateArchetype(newSignature, sharedId);
    
    // 4. 엔티티 이동 (필요한 경우)
    if (oldLocation.archetype && oldLocation.archetype != newArchetype) {
        MoveEntityBetweenArchetypes(entity, oldLocation.archetype, newArchetype);
    } else if (!oldLocation.archetype) {
        // 새 엔티티인 경우
        size_t newIndex = newArchetype->AddEntity(entity);
        mEntityLocations[entity] = {newArchetype, newIndex};
    }
    
    // 5. 새 컴포넌트 데이터 추가
    newArchetype->AddComponentData<T>(component);
}
```

#### 데이터 이동 최적화
```cpp
void MoveEntityBetweenArchetypes(Entity entity, Archetype* source, Archetype* destination) {
    size_t oldIndex = mEntityLocations[entity].index;
    size_t newIndex = destination->AddEntity(entity);
    
    // 1. 공통 컴포넌트 데이터 복사
    const auto& sourceSignature = source->GetSignature();
    for (ComponentType i = 0; i < MAX_COMPONENTS; ++i) {
        if (sourceSignature.test(i)) {
            IComponentArray* sourceArray = source->GetComponentArray(i);
            IComponentArray* destArray = destination->GetComponentArray(i);
            
            // 데이터 이동 (복사 후 원본은 제거됨)
            sourceArray->MoveData(oldIndex, destArray);
        }
    }
    
    // 2. 기존 위치에서 엔티티 제거
    Entity movedEntity = source->RemoveEntity(entity);
    
    // 3. 위치 정보 업데이트
    mEntityLocations[entity] = {destination, newIndex};
    if (movedEntity != INVALID_ENTITY) {
        mEntityLocations[movedEntity].index = oldIndex;
    }
}
```

### 공유 컴포넌트 (Shared Components)

#### 개념과 활용
```cpp
// 공유 컴포넌트: 여러 엔티티가 동일한 값을 공유
struct SharedRenderProperties {
    MaterialID materialId;
    ShaderID shaderId;
    bool castShadows;
    
    bool operator==(const SharedRenderProperties& other) const {
        return materialId == other.materialId && 
               shaderId == other.shaderId && 
               castShadows == other.castShadows;
    }
};

// 해시 함수
struct SharedRenderPropertiesHasher {
    size_t operator()(const SharedRenderProperties& props) const {
        return std::hash<MaterialID>{}(props.materialId) ^
               std::hash<ShaderID>{}(props.shaderId) ^
               std::hash<bool>{}(props.castShadows);
    }
};
```

#### 공유 컴포넌트 관리
```cpp
// 같은 머티리얼을 사용하는 엔티티들은 같은 Archetype에 그룹화
SharedRenderProperties metalProps = {materialId: 5, shaderId: 2, castShadows: true};
SharedRenderProperties woodProps = {materialId: 3, shaderId: 1, castShadows: false};

// Archetype 분리:
// Archetype1: Transform+Mesh+Render (metalProps 공유)
// Archetype2: Transform+Mesh+Render (woodProps 공유)
```

### 성능 최적화 기법

#### 1. **배치 렌더링 최적화**
```cpp
void OptimizedRenderSystem::Update() {
    // 공유 컴포넌트별로 그룹화된 Archetype들을 순회
    for (auto* archetype : GetRenderableArchetypes()) {
        auto sharedProps = GetSharedComponent<SharedRenderProperties>(archetype);
        
        // 동일한 머티리얼/셰이더를 사용하는 오브젝트들을 배치 처리
        SetMaterial(sharedProps.materialId);
        SetShader(sharedProps.shaderId);
        
        // 인스턴스 데이터 일괄 업로드
        auto* transforms = archetype->GetComponentArray<TransformComponent>();
        auto* meshes = archetype->GetComponentArray<MeshComponent>();
        
        RenderInstanced(transforms, meshes, archetype->GetEntityCount());
    }
}
```

#### 2. **메모리 접근 패턴 최적화**
```cpp
// SIMD 친화적인 데이터 레이아웃
struct TransformComponent_SIMD {
    alignas(16) float posX[4];  // 4개씩 묶어서 SIMD 처리
    alignas(16) float posY[4];
    alignas(16) float posZ[4];
    // ...
};

// 벡터화된 처리
void ProcessTransforms_SIMD(TransformComponent_SIMD* data, size_t count) {
    for (size_t i = 0; i < count; i += 4) {
        __m128 x = _mm_load_ps(&data->posX[i]);
        __m128 y = _mm_load_ps(&data->posY[i]);
        __m128 z = _mm_load_ps(&data->posZ[i]);
        
        // SIMD 연산으로 4개씩 동시 처리
        ProcessPositions_SIMD(x, y, z);
    }
}
```

#### 3. **Archetype 크기 최적화**
```cpp
// 적절한 Archetype 크기 유지
class ArchetypeOptimizer {
public:
    void OptimizeArchetypes() {
        for (auto* archetype : GetAllArchetypes()) {
            size_t entityCount = archetype->GetEntityCount();
            
            // 너무 작은 Archetype은 병합 고려
            if (entityCount < 16) {
                ConsiderMerging(archetype);
            }
            
            // 너무 큰 Archetype은 분할 고려
            if (entityCount > 10000) {
                ConsiderSplitting(archetype);
            }
            
            // 메모리 사용량 최적화
            archetype->ShrinkToFit();
        }
    }
};
```

### 디버깅 및 프로파일링

#### Archetype 상태 모니터링
```cpp
void ArchetypeManager::PrintDebugInfo() {
    std::cout << "=== Archetype Debug Info ===" << std::endl;
    
    for (auto& [signature, sharedMap] : mArchetypes) {
        std::cout << "Signature: " << signature << std::endl;
        
        for (auto& [sharedId, archetype] : sharedMap) {
            std::cout << "  SharedID: " << sharedId 
                      << ", Entities: " << archetype->GetEntityCount() << std::endl;
            
            // 메모리 사용량 출력
            size_t memoryUsage = CalculateMemoryUsage(archetype.get());
            std::cout << "  Memory: " << memoryUsage << " bytes" << std::endl;
        }
    }
}
```

#### 성능 메트릭
```cpp
struct ArchetypeMetrics {
    size_t totalArchetypes;
    size_t totalEntities;
    size_t averageArchetypeSize;
    size_t memoryUsage;
    float cacheHitRatio;
    float migrationFrequency;
};

ArchetypeMetrics CollectMetrics() {
    // 성능 지표 수집 및 분석
    return {
        .totalArchetypes = CountArchetypes(),
        .averageArchetypeSize = CalculateAverageSize(),
        .cacheHitRatio = MeasureCachePerformance(),
        // ...
    };
}
```

---

## 코어 컴포넌트

### 1. TransformComponent
**파일**: `TransformComponent.h`
**용도**: 3D 공간에서의 위치, 회전, 크기 정보

```cpp
struct TransformComponent {
    static const char* GetName() { return "TransformComponent"; }
    
    float3 Position;        // 월드 위치 (X, Y, Z)
    float3 Scale;           // 스케일 (X, Y, Z)
    float3 Rotation;        // 오일러 각도 (Pitch, Yaw, Roll)
    float4 RotationQuat;    // 쿼터니언 회전 (더 정확한 회전)
    
    bool Dirty = true;      // 변경 감지 플래그
};
```

#### 사용 시나리오
- **모든 3D 오브젝트**의 기본 컴포넌트
- **WorldMatrixUpdateSystem**에서 행렬 계산
- **렌더링 시스템**에서 월드 변환 적용

#### 사용 예제
```cpp
// 오브젝트 생성 및 위치 설정
Entity cube = gCoordinator.CreateEntity();
gCoordinator.AddComponent<TransformComponent>(cube, {
    .Position = {5.0f, 0.0f, 0.0f},
    .Scale = {1.0f, 1.0f, 1.0f},
    .Rotation = {0.0f, 45.0f, 0.0f}  // Y축 45도 회전
});

// 런타임 위치 변경
auto& transform = gCoordinator.GetComponent<TransformComponent>(cube);
transform.Position.x += deltaTime * speed;
transform.Dirty = true;  // 변경 사항 표시
```

### 2. TimeComponent (싱글톤)
**파일**: `TimeComponent.h`
**용도**: 전역 시간 관리

```cpp
struct TimeComponent {
    static const char* GetName() { return "TimeComponent"; }
    
    float deltaTime = 0.0f;     // 프레임 간 시간 (초)
    float totalTime = 0.0f;     // 게임 시작부터 총 시간
    float timeScale = 1.0f;     // 시간 배율 (슬로우 모션 등)
    uint32_t frameCount = 0;    // 총 프레임 수
    float fps = 0.0f;          // 현재 FPS
    
    // 내부 계산용
    float accumulatedTime = 0.0f;
    int frameCountForFPS = 0;
};
```

#### 사용 시나리오
- **물리 시뮬레이션**에서 시간 기반 계산
- **애니메이션 시스템**에서 시간 진행
- **게임플레이 로직**에서 타이머 구현

---

## 렌더링 컴포넌트

### 1. DX12_RenderComponent
**파일**: `DX12_RenderComponent.h`
**용도**: DirectX12 렌더링 제어

```cpp
struct DX12_RenderComponent {
    static const char* GetName() { return "DX12_RenderComponent"; }
    
    bool isVisible = true;          // 렌더링 여부
    uint32_t renderQueue = 0;       // 렌더링 순서 (낮을수록 먼저)
    eRenderLayer layer = eRenderLayer::Opaque;  // 렌더링 레이어
    float alpha = 1.0f;            // 투명도
    bool castShadows = true;       // 그림자 생성 여부
    bool receiveShadows = true;    // 그림자 받기 여부
};

enum class eRenderLayer : uint32_t {
    Background = 0,    // 배경 (스카이박스 등)
    Opaque = 1000,     // 불투명 오브젝트
    Transparent = 2000, // 투명 오브젝트
    Overlay = 3000,    // 오버레이 (UI 등)
    Sprite = 4000      // 2D 스프라이트
};
```

### 2. DX12_MeshComponent
**파일**: `DX12_MeshComponent.h`
**용도**: 메시 리소스 참조

```cpp
struct DX12_MeshComponent {
    static const char* GetName() { return "DX12_MeshComponent"; }
    
    RepoHandle meshHandle;          // MeshRepository 핸들
    BoundingBox boundingBox;        // 로컬 바운딩 박스
    uint32_t submeshIndex = 0;      // 서브메시 인덱스
    bool enableLOD = false;         // LOD 사용 여부
    float lodDistance = 100.0f;     // LOD 전환 거리
};
```

### 3. MaterialComponent
**파일**: `MaterialComponent.h`
**용도**: PBR 머티리얼 정의

```cpp
// 머티리얼 속성
struct MaterialPropertyComponent {
    Vector4 DiffuseAlbedo{0.8f, 0.8f, 0.8f, 1.0f};  // 기본 색상
    Vector3 FresnelR0{0.04f, 0.04f, 0.04f};         // 프레넬 반사율
    float Roughness = 0.5f;                          // 거칠기 (0=매끄러움, 1=거침)
    float Metallic = 0.0f;                          // 금속성 (0=비금속, 1=금속)
};

// 텍스처 인덱스
struct MaterialIndexComponent {
    int DiffMapIndex = -1;      // 디퓨즈 맵
    int NormMapIndex = -1;      // 노말 맵
    int AOMapIndex = -1;        // Ambient Occlusion 맵
    int MetallicMapIndex = -1;  // 메탈릭 맵
    int RoughnessMapIndex = -1; // 러프니스 맵
    int EmissiveMapIndex = -1;  // 이미시브 맵
    uint32_t Flags = 0;         // 사용 플래그
};

// 플래그 정의
enum eMaterialFlags : uint32_t {
    USE_DIFFUSE_MAP = 1 << 0,
    USE_NORMAL_MAP = 1 << 1,
    INVERT_NORMAL_MAP = 1 << 2,
    USE_AO_MAP = 1 << 3,
    USE_METALLIC_MAP = 1 << 4,
    USE_ROUGHNESS_MAP = 1 << 5,
    USE_EMISSIVE_MAP = 1 << 6,
    ALPHA_TEST = 1 << 7
};

// UV 변환
struct MaterialUVTransformComponent {
    Matrix MatTransform = Matrix::Identity;  // UV 좌표 변환 행렬
};
```

#### 머티리얼 사용 예제
```cpp
Entity metalSphere = gCoordinator.CreateEntity();

// 금속 재질 설정
gCoordinator.AddComponent<MaterialPropertyComponent>(metalSphere, {
    .DiffuseAlbedo = {0.9f, 0.7f, 0.1f, 1.0f},  // 황금색
    .FresnelR0 = {0.95f, 0.64f, 0.54f},         // 금의 프레넬 반사율
    .Roughness = 0.1f,                          // 매끄러운 표면
    .Metallic = 1.0f                            // 완전한 금속
});

// 텍스처 설정
gCoordinator.AddComponent<MaterialIndexComponent>(metalSphere, {
    .DiffMapIndex = TextureSystem::LoadTexture("gold_albedo.png"),
    .NormMapIndex = TextureSystem::LoadTexture("gold_normal.png"),
    .Flags = USE_DIFFUSE_MAP | USE_NORMAL_MAP
});
```

### 4. LightComponent
**파일**: `LightComponent.h`
**용도**: 조명 정의

```cpp
struct LightComponent {
    static const char* GetName() { return "LightComponent"; }
    
    Vector3 Strength{0.5f, 0.5f, 0.5f};        // 조명 강도 (RGB)
    Vector3 Direction{0.0f, -1.0f, 0.0f};      // 방향 (방향광/스포트라이트용)
    Vector3 Position{0.0f, 0.0f, 0.0f};       // 위치 (점광원/스포트라이트용)
    
    float FalloffStart = 1.0f;                 // 감쇠 시작 거리
    float FalloffEnd = 10.0f;                  // 감쇠 끝 거리
    float SpotPower = 64.0f;                   // 스포트라이트 집중도
    
    eLightType type = eLightType::Point;       // 조명 타입
    float radius = 1.0f;                       // 조명 반지름
    float haloRadius = 1.0f;                   // 헤일로 반지름
    float haloStrength = 1.0f;                 // 헤일로 강도
};

enum class eLightType : uint32_t {
    Directional = 0,    // 방향광 (태양광)
    Point = 1,          // 점광원
    Spot = 2            // 스포트라이트
};
```

#### 조명 사용 예제
```cpp
// 방향광 (태양) 생성
Entity sun = gCoordinator.CreateEntity();
gCoordinator.AddComponent<LightComponent>(sun, {
    .Strength = {2.0f, 1.8f, 1.5f},           // 따뜻한 햇빛
    .Direction = {0.3f, -0.8f, 0.5f},         // 사선 방향
    .type = eLightType::Directional
});

// 점광원 (횃불) 생성
Entity torch = gCoordinator.CreateEntity();
gCoordinator.AddComponent<LightComponent>(torch, {
    .Strength = {1.0f, 0.6f, 0.2f},          // 오렌지색 불빛
    .Position = {10.0f, 3.0f, 5.0f},         // 횃불 위치
    .FalloffStart = 2.0f,
    .FalloffEnd = 8.0f,
    .type = eLightType::Point
});
```

---

## 물리 컴포넌트

### 1. RigidBodyComponent
**파일**: `RigidBodyComponent.h`
**용도**: 물리 시뮬레이션용 강체 정보

```cpp
struct RigidBodyComponent {
    static const char* GetName() { return "RigidBodyComponent"; }
    
    // 기본 물리 속성
    float Mass = 1.0f;                          // 질량 (kg)
    
    // 운동 상태
    float3 DiffPosition{0.0f, 0.0f, 0.0f};     // 위치 변화량
    float3 Velocity{0.0f, 0.0f, 0.0f};         // 선형 속도 (m/s)
    float3 AngularVelocity{0.0f, 0.0f, 0.0f};  // 각속도 (rad/s)
    
    // 힘과 토크
    float3 Force{0.0f, 0.0f, 0.0f};           // 적용된 힘 (N)
    float3 Torque{0.0f, 0.0f, 0.0f};          // 적용된 토크 (N⋅m)
    float3 Acceleration{0.0f, 0.0f, 0.0f};     // 가속도 (m/s²)
    float3 AngularAcceleration{0.0f, 0.0f, 0.0f}; // 각가속도 (rad/s²)
    
    // 물리 설정
    bool UseGravity = true;                     // 중력 적용 여부
    bool IsKinematic = false;                   // 키네마틱 오브젝트 여부
};
```

#### 물리 시뮬레이션 예제
```cpp
// 떨어지는 공 생성
Entity ball = gCoordinator.CreateEntity();
gCoordinator.AddComponent<TransformComponent>(ball, {
    .Position = {0.0f, 10.0f, 0.0f},
    .Scale = {0.5f, 0.5f, 0.5f}
});
gCoordinator.AddComponent<RigidBodyComponent>(ball, {
    .Mass = 2.0f,                              // 2kg
    .UseGravity = true                         // 중력 적용
});
gCoordinator.AddComponent<GravityComponent>(ball, {
    .Force = {0.0f, -9.81f, 0.0f}            // 지구 중력
});

// 키네마틱 플랫폼 생성 (물리 영향을 받지 않음)
Entity platform = gCoordinator.CreateEntity();
gCoordinator.AddComponent<RigidBodyComponent>(platform, {
    .IsKinematic = true,                       // 외부 힘에 영향받지 않음
    .UseGravity = false
});
```

### 2. GravityComponent
**파일**: `GravityComponent.h`
**용도**: 중력 정의

```cpp
struct GravityComponent {
    static const char* GetName() { return "GravityComponent"; }
    
    float3 Force{0.0f, -9.81f, 0.0f};         // 중력 벡터 (m/s²)
};
```

### 3. DX12_BoundingComponent
**파일**: `DX12_BoundingComponent.h`
**용도**: 충돌 검출 및 컬링용 바운딩 볼륨

```cpp
struct DX12_BoundingComponent {
    static const char* GetName() { return "DX12_BoundingComponent"; }
    
    BoundingBox localBounds;    // 로컬 공간 바운딩 박스
    BoundingBox worldBounds;    // 월드 공간 바운딩 박스
    BoundingSphere sphere;      // 바운딩 스피어
    
    bool isVisible = true;      // 컬링 결과
    bool needsUpdate = true;    // 업데이트 필요 여부
};
```

---

## 게임플레이 컴포넌트

### 1. PlayerControlComponent
**파일**: `PlayerControlComponent.h`
**용도**: 플레이어 제어 설정

```cpp
struct PlayerControlComponent {
    static const char* GetName() { return "PlayerControlComponent"; }
    
    // 이동 설정
    float moveSpeed = 5.0f;         // 이동 속도 (m/s)
    float runMultiplier = 2.0f;     // 달리기 배율
    float jumpForce = 8.0f;         // 점프 힘
    
    // 회전 설정
    float mouseSensitivity = 2.0f;  // 마우스 감도
    float maxLookAngle = 90.0f;     // 최대 시야각
    
    // 상태
    bool isGrounded = true;         // 땅에 있는지 여부
    bool isRunning = false;         // 달리고 있는지 여부
    bool canJump = true;            // 점프 가능 여부
    
    // 입력 매핑
    int forwardKey = 'W';          // 전진 키
    int backwardKey = 'S';         // 후진 키
    int leftKey = 'A';             // 좌측 이동 키
    int rightKey = 'D';            // 우측 이동 키
    int jumpKey = VK_SPACE;        // 점프 키
    int runKey = VK_LSHIFT;        // 달리기 키
};
```

### 2. CameraComponent
**파일**: `CameraComponent.h`
**용도**: 카메라 설정

```cpp
struct CameraComponent {
    static const char* GetName() { return "CameraComponent"; }
    
    // 투영 설정
    float fovY = 60.0f;            // 세로 시야각 (도)
    float aspectRatio = 16.0f/9.0f; // 화면 비율
    float nearPlane = 0.1f;        // 근평면 거리
    float farPlane = 1000.0f;      // 원평면 거리
    
    // 뷰 설정
    Vector3 target{0.0f, 0.0f, 0.0f};    // 바라보는 지점
    Vector3 up{0.0f, 1.0f, 0.0f};       // 업 벡터
    
    // 카메라 타입
    eCameraType type = eCameraType::Perspective;
    
    // 직교 투영용 (2D 카메라)
    float orthoWidth = 10.0f;
    float orthoHeight = 10.0f;
    
    // 상태
    bool isMainCamera = false;     // 메인 카메라 여부
    bool isActive = true;          // 활성화 여부
};

enum class eCameraType {
    Perspective,    // 원근 투영 (3D)
    Orthographic    // 직교 투영 (2D)
};
```

---

## 오디오 컴포넌트

### 1. FMODAudioComponent
**파일**: `FMODAudioComponent.h`
**용도**: FMOD 오디오 재생

```cpp
struct FMODAudioComponent {
    static const char* GetName() { return "FMODAudioComponent"; }
    
    RepoHandle handle;              // 오디오 파일 핸들
    
    // 재생 설정
    float volume = 1.0f;           // 볼륨 (0.0 ~ 1.0)
    float pitch = 1.0f;            // 피치 (0.5 ~ 2.0)
    float pan = 0.0f;              // 패닝 (-1.0 = 왼쪽, 1.0 = 오른쪽)
    
    // 3D 오디오 설정
    bool is3D = false;             // 3D 사운드 여부
    float minDistance = 1.0f;      // 최소 거리 (3D용)
    float maxDistance = 100.0f;    // 최대 거리 (3D용)
    
    // 재생 제어
    bool loop = false;             // 반복 재생
    bool autoPlay = false;         // 자동 재생
    bool playOnce = false;         // 한 번만 재생
    
    // 상태
    bool isPlaying = false;        // 재생 중 여부
    bool isPaused = false;         // 일시정지 여부
    float currentTime = 0.0f;      // 현재 재생 시간
    float totalTime = 0.0f;        // 총 재생 시간
};
```

#### 오디오 사용 예제
```cpp
// 배경음악 설정
Entity bgm = gCoordinator.CreateEntity();
gCoordinator.AddComponent<FMODAudioComponent>(bgm, {
    .handle = FMODAudioRepository::LoadSound("bgm/main_theme.mp3"),
    .volume = 0.7f,
    .loop = true,
    .autoPlay = true
});

// 3D 효과음 설정
Entity explosion = gCoordinator.CreateEntity();
gCoordinator.AddComponent<TransformComponent>(explosion, {
    .Position = {10.0f, 0.0f, 5.0f}
});
gCoordinator.AddComponent<FMODAudioComponent>(explosion, {
    .handle = FMODAudioRepository::LoadSound("sfx/explosion.wav"),
    .volume = 1.0f,
    .is3D = true,
    .minDistance = 5.0f,
    .maxDistance = 50.0f,
    .playOnce = true
});
```

---

## UI/입력 컴포넌트

### 1. InputComponent
**파일**: `InputComponent.h`
**용도**: 입력 이벤트 처리

```cpp
struct InputComponent {
    static const char* GetName() { return "InputComponent"; }
    
    // 키보드 입력
    std::vector<int> watchedKeys;       // 감시할 키 목록
    std::vector<int> pressedKeys;       // 이번 프레임에 눌린 키
    std::vector<int> releasedKeys;      // 이번 프레임에 떨어진 키
    
    // 마우스 입력
    bool watchMouse = false;            // 마우스 감시 여부
    Vector2 mousePosition{0.0f, 0.0f}; // 마우스 위치
    Vector2 mouseDelta{0.0f, 0.0f};    // 마우스 이동량
    int mouseButtons = 0;               // 마우스 버튼 상태 (비트마스크)
    
    // 콜백 함수 (옵션)
    std::function<void(int)> onKeyPressed;
    std::function<void(int)> onKeyReleased;
    std::function<void(Vector2)> onMouseMove;
};
```

### 2. ImGuiComponent
**파일**: `ImGuiComponent.h`
**용도**: ImGui UI 요소

```cpp
struct ImGuiComponent {
    static const char* GetName() { return "ImGuiComponent"; }
    
    std::string windowTitle = "UI Window";  // 윈도우 제목
    bool isVisible = true;                  // 표시 여부
    bool hasMenuBar = false;               // 메뉴바 포함 여부
    bool isResizable = true;               // 크기 조절 가능 여부
    
    // 윈도우 설정
    Vector2 position{100.0f, 100.0f};     // 윈도우 위치
    Vector2 size{300.0f, 200.0f};         // 윈도우 크기
    
    // 렌더링 콜백
    std::function<void()> renderCallback;   // UI 렌더링 함수
};
```

---

## 시스템별 컴포넌트 조합

### 렌더링 가능한 3D 오브젝트
```cpp
Entity renderableObject = gCoordinator.CreateEntity();

// 필수 컴포넌트
gCoordinator.AddComponent<TransformComponent>(renderableObject, {...});
gCoordinator.AddComponent<DX12_MeshComponent>(renderableObject, {...});
gCoordinator.AddComponent<DX12_RenderComponent>(renderableObject, {...});

// 선택적 컴포넌트
gCoordinator.AddComponent<MaterialPropertyComponent>(renderableObject, {...});
gCoordinator.AddComponent<DX12_BoundingComponent>(renderableObject, {...});
```

### 물리 시뮬레이션 오브젝트
```cpp
Entity physicsObject = gCoordinator.CreateEntity();

// 필수 컴포넌트
gCoordinator.AddComponent<TransformComponent>(physicsObject, {...});
gCoordinator.AddComponent<RigidBodyComponent>(physicsObject, {...});

// 중력 적용
gCoordinator.AddComponent<GravityComponent>(physicsObject, {...});

// 충돌 검출
gCoordinator.AddComponent<DX12_BoundingComponent>(physicsObject, {...});
```

### 플레이어 캐릭터
```cpp
Entity player = gCoordinator.CreateEntity();

// 기본 컴포넌트
gCoordinator.AddComponent<TransformComponent>(player, {...});
gCoordinator.AddComponent<PlayerControlComponent>(player, {...});
gCoordinator.AddComponent<RigidBodyComponent>(player, {...});

// 렌더링
gCoordinator.AddComponent<DX12_MeshComponent>(player, {...});
gCoordinator.AddComponent<DX12_RenderComponent>(player, {...});

// 입력 처리
gCoordinator.AddComponent<InputComponent>(player, {...});
```

### 조명이 있는 씬
```cpp
Entity lightSource = gCoordinator.CreateEntity();

// 조명 설정
gCoordinator.AddComponent<TransformComponent>(lightSource, {...});
gCoordinator.AddComponent<LightComponent>(lightSource, {...});

// 선택적: 조명 시각화용 메시
gCoordinator.AddComponent<DX12_MeshComponent>(lightSource, {...});
gCoordinator.AddComponent<DX12_RenderComponent>(lightSource, {...});
```

---

## 커스텀 컴포넌트 생성 가이드

### 1. 컴포넌트 구조체 정의

```cpp
// MyComponent.h
#pragma once
#include "ECSConfig.h"

struct MyCustomComponent {
    // 1. 이름 정의 (필수)
    static const char* GetName() { return "MyCustomComponent"; }
    
    // 2. 데이터 멤버들
    float health = 100.0f;
    bool isInvulnerable = false;
    std::string statusEffect = "";
    
    // 3. 기본값 설정
    Vector3 lastDamageDirection{0.0f, 0.0f, 0.0f};
};
```

### 2. JSON 직렬화 구현 (선택적)

```cpp
// JSON 저장
inline void to_json(json& j, const MyCustomComponent& p) {
    j = json{
        {"health", p.health},
        {"isInvulnerable", p.isInvulnerable},
        {"statusEffect", p.statusEffect},
        {"lastDamageDirection", {
            p.lastDamageDirection.x,
            p.lastDamageDirection.y,
            p.lastDamageDirection.z
        }}
    };
}

// JSON 로드
inline void from_json(const json& j, MyCustomComponent& p) {
    j.at("health").get_to(p.health);
    j.at("isInvulnerable").get_to(p.isInvulnerable);
    j.at("statusEffect").get_to(p.statusEffect);
    
    auto dir = j.at("lastDamageDirection");
    p.lastDamageDirection.x = dir[0];
    p.lastDamageDirection.y = dir[1];
    p.lastDamageDirection.z = dir[2];
}
```

### 3. 컴포넌트 등록

```cpp
// 초기화 시 등록
void RegisterCustomComponents() {
    gCoordinator.RegisterComponent<MyCustomComponent>();
}
```

### 4. 시스템에서 사용

```cpp
class HealthSystem : public ECS::ISystem {
public:
    void Update() override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        
        for (ECS::Entity entity : mEntities) {
            auto& health = coordinator.GetComponent<MyCustomComponent>(entity);
            
            // 체력 로직 처리
            if (health.health <= 0.0f && !health.isInvulnerable) {
                // 사망 처리
                DestroyEntity(entity);
            }
        }
    }
};

// 시스템 등록 및 시그니처 설정
void RegisterHealthSystem() {
    auto healthSystem = gCoordinator.RegisterSystem<HealthSystem>();
    
    ECS::Signature signature;
    signature.set(gCoordinator.GetComponentType<MyCustomComponent>());
    signature.set(gCoordinator.GetComponentType<TransformComponent>());
    
    gCoordinator.SetSystemSignature<HealthSystem>(signature);
}
```

### 5. 실제 사용

```cpp
// 적 생성
Entity enemy = gCoordinator.CreateEntity();
gCoordinator.AddComponent<MyCustomComponent>(enemy, {
    .health = 50.0f,
    .isInvulnerable = false,
    .statusEffect = "poison"
});

// 런타임에서 수정
auto& health = gCoordinator.GetComponent<MyCustomComponent>(enemy);
health.health -= damageAmount;
health.lastDamageDirection = normalize(playerPos - enemyPos);
```

---

## 컴포넌트 최적화 팁

### 1. 메모리 레이아웃 최적화
```cpp
// ❌ 나쁜 예: 메모리 정렬 비효율
struct BadComponent {
    bool flag1;      // 1 byte
    float value1;    // 4 bytes (3 bytes padding)
    bool flag2;      // 1 byte
    double value2;   // 8 bytes (7 bytes padding)
};

// ✅ 좋은 예: 메모리 정렬 최적화
struct GoodComponent {
    double value2;   // 8 bytes (먼저 배치)
    float value1;    // 4 bytes
    bool flag1;      // 1 byte
    bool flag2;      // 1 byte (2 bytes padding)
};
```

### 2. 불필요한 데이터 분리
```cpp
// ❌ 모든 데이터를 하나의 컴포넌트에
struct MonolithicComponent {
    // 렌더링 관련
    bool isVisible;
    float alpha;
    
    // 물리 관련
    float mass;
    Vector3 velocity;
    
    // 오디오 관련
    float volume;
    bool loop;
};

// ✅ 기능별로 컴포넌트 분리
struct RenderComponent { bool isVisible; float alpha; };
struct PhysicsComponent { float mass; Vector3 velocity; };
struct AudioComponent { float volume; bool loop; };
```

### 3. 상태 변경 감지 최적화
```cpp
struct OptimizedComponent {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
    
    // 변경 감지용 플래그
    mutable bool isDirty = true;
    
    // 설정 시 자동으로 dirty 플래그 설정
    void SetPosition(const Vector3& pos) {
        if (position != pos) {
            position = pos;
            isDirty = true;
        }
    }
};
```

---

이 가이드를 통해 ECS 시스템의 모든 컴포넌트를 이해하고 활용할 수 있습니다. 필요에 따라 새로운 컴포넌트를 추가하거나 기존 컴포넌트를 확장하여 게임의 요구사항에 맞게 커스터마이징할 수 있습니다.
