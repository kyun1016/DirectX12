# ECS 시스템별 상세 가이드

## 목차
1. [코어 시스템](#코어-시스템)
2. [DirectX12 렌더링 시스템](#directx12-렌더링-시스템)
3. [입출력 시스템](#입출력-시스템)
4. [물리 및 게임플레이 시스템](#물리-및-게임플레이-시스템)
5. [리소스 관리 시스템](#리소스-관리-시스템)
6. [오디오 시스템](#오디오-시스템)
7. [개발자 도구 시스템](#개발자-도구-시스템)

> 📋 **컴포넌트 상세 가이드**: 모든 컴포넌트의 구조와 사용법은 **[COMPONENTS_GUIDE.md](./COMPONENTS_GUIDE.md)**를 참조하세요.

---

## 코어 시스템

### 1. ECS SystemManager
**파일**: `ECSSystem.h`
**역할**: 모든 시스템의 생명주기를 관리하는 핵심 시스템

#### 주요 기능
```cpp
- RegisterSystem<T>()     // 새로운 시스템 등록
- SetSignature<T>()       // 시스템별 컴포넌트 시그니처 설정
- EntityDestroyed()       // 엔티티 삭제 시 모든 시스템에서 제거
- EntitySignatureChanged() // 엔티티 컴포넌트 변경 시 시스템 재분배
```

#### 생명주기 관리
```cpp
BeginPlayAllSystems()    // 게임 시작 시 모든 시스템 초기화
SyncAllSystems()         // 동기화 단계 (윈도우, 입력 등)
PreUpdateAllSystems()    // 전처리 업데이트
UpdateAllSystems()       // 메인 게임 로직
LateUpdateAllSystems()   // 후처리 업데이트
FixedUpdateAllSystems()  // 고정 시간 업데이트 (물리 등)
FinalUpdateAllSystems()  // 최종 업데이트 (렌더링 준비)
EndPlayAllSystems()      // 게임 종료 시 정리
```

#### 병렬 처리
- `std::async`를 사용한 시스템별 병렬 실행
- `std::future`로 작업 완료 대기
- 각 업데이트 단계에서 독립적으로 병렬 처리

---

## DirectX12 렌더링 시스템

### 1. DX12_DeviceSystem
**역할**: DirectX12 디바이스 및 기본 리소스 관리

#### 주요 기능
```cpp
- DirectX12 디바이스 생성 및 초기화
- 어댑터 열거 및 최적 GPU 선택
- 피처 레벨 확인 및 설정
- 디바이스 기능 쿼리 (Tiled Resources, Conservative Rasterization 등)
```

### 2. DX12_CommandSystem
**역할**: 커맨드 큐, 커맨드 리스트 관리

#### 주요 기능
```cpp
- 커맨드 큐 생성 (Graphics, Compute, Copy)
- 커맨드 리스트 할당 및 관리
- 커맨드 제출 및 동기화
- GPU 펜스를 통한 동기화
```

### 3. DX12_SwapChainSystem
**역할**: 스왑체인 및 백버퍼 관리

#### 주요 기능
```cpp
- DXGI 스왑체인 생성 및 구성
- 백버퍼 리소스 관리
- Present() 호출 및 V-Sync 제어
- 해상도 변경 지원
```

### 4. DX12_RenderSystem
**파일**: `DX12_RenderSystem.h`
**역할**: 실제 렌더링 파이프라인 실행

#### 주요 기능
```cpp
virtual void Sync() override {
    CameraSystem::GetInstance().Sync();                    // 카메라 업데이트
    DX12_SceneSystem::GetInstance().UpdateInstance();      // 인스턴스 데이터 업데이트
    DX12_FrameResourceSystem::GetInstance().BeginFrame();  // 프레임 시작
}

virtual void Update() override {
    BeginRenderPass();                                      // 렌더패스 시작
    DrawRenderItems(eRenderLayer::Sprite);                 // 레이어별 렌더링
    EndRenderPass();                                        // 렌더패스 종료
    ImGuiSystem::GetInstance().Render();                   // UI 렌더링
    DX12_CommandSystem::GetInstance().ExecuteCommandList(); // 커맨드 실행
    DX12_SwapChainSystem::GetInstance().Present(false);    // 화면 출력
    DX12_FrameResourceSystem::GetInstance().EndFrame();    // 프레임 종료
}
```

#### 렌더링 레이어
```cpp
enum class eRenderLayer {
    Opaque,     // 불투명 오브젝트
    Transparent,// 투명 오브젝트
    Sprite,     // 2D 스프라이트
    UI,         // 사용자 인터페이스
    Test        // 테스트용
};
```

### 5. DX12_PSOSystem
**역할**: 파이프라인 상태 객체(PSO) 관리

#### 주요 기능
```cpp
- 그래픽스 파이프라인 상태 생성
- 컴퓨트 파이프라인 상태 생성
- PSO 캐싱 및 재사용
- 동적 상태 변경 지원
```

### 6. DX12_MeshSystem
**역할**: 메시 데이터 및 버텍스 버퍼 관리

#### 주요 기능
```cpp
- 버텍스 버퍼 생성 및 관리
- 인덱스 버퍼 생성 및 관리
- 메시 데이터 업로드
- GPU 메모리 최적화
```

### 7. DX12_FrameResourceSystem
**역할**: 프레임별 리소스 관리 (더블/트리플 버퍼링)

#### 주요 기능
```cpp
- 프레임별 상수 버퍼 관리
- GPU 메모리 동적 할당
- 프레임 동기화
- 리소스 재사용 최적화
```

### 8. DX12_ShaderCompileSystem
**역할**: 셰이더 컴파일 및 관리

#### 주요 기능
```cpp
- HLSL 셰이더 컴파일
- 셰이더 캐싱
- 런타임 셰이더 리로딩
- 컴파일 에러 처리
```

### 9. DX12_RootSignatureSystem
**역할**: 루트 시그니처 관리

#### 주요 기능
```cpp
- 루트 시그니처 생성 및 캐싱
- 디스크립터 테이블 관리
- 상수 버퍼 바인딩
- 텍스처 리소스 바인딩
```

### 10. DX12_SceneSystem
**역할**: 씬 데이터 및 인스턴스 관리

#### 주요 기능
```cpp
- 인스턴스 데이터 업데이트
- 씬 그래프 관리
- 컬링 및 LOD 관리
- 배치 렌더링 최적화
```

---

## 입출력 시스템

### 1. WindowSystem
**파일**: `WindowSystem.h`
**역할**: 윈도우 생성 및 메시지 처리

#### 주요 기능
```cpp
- 윈도우 클래스 등록 및 생성
- Win32 메시지 루프 처리
- 윈도우 크기 조정 및 이벤트 처리
- ImGui와의 메시지 통합
```

#### 메시지 처리
```cpp
bool Sync() {
    MSG msg = {};
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.message != WM_QUIT;
}

LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // ImGui 우선 처리
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return true;
    
    // 게임 입력 처리
    // 윈도우 이벤트 처리
}
```

### 2. InputSystem
**파일**: `InputSystem.h`
**역할**: 키보드, 마우스, 게임패드 입력 처리

#### 주요 기능
```cpp
// 키보드 입력
bool IsKeyDown(int vk_code)      // 키가 눌려있는 동안
bool IsKeyPressed(int vk_code)   // 키가 처음 눌린 순간
bool IsKeyReleased(int vk_code)  // 키를 뗀 순간

// 마우스 입력
bool IsMouseDown(MouseButton button)
bool IsMousePressed(MouseButton button)
bool IsMouseReleased(MouseButton button)
Vector2 GetMousePosition()
Vector2 GetMouseDelta()

// 게임패드 입력
bool IsGamepadConnected(int playerIndex)
bool IsGamepadButtonDown(int playerIndex, GamepadButton button)
float GetGamepadTrigger(int playerIndex, GamepadTrigger trigger)
Vector2 GetGamepadStick(int playerIndex, GamepadStick stick)
```

#### 입력 상태 관리
```cpp
void PreUpdate() {
    UpdateKeyboardState();  // 키보드 상태 갱신
    UpdateMouseState();     // 마우스 상태 갱신
    UpdateGamepadStates();  // 게임패드 상태 갱신
}
```

---

## 물리 및 게임플레이 시스템

### 1. PhysicsSystem
**파일**: `PhysicsSystem.h`
**역할**: 기본 물리 시뮬레이션

#### 주요 기능
```cpp
void Update() override {
    // 가속도 → 속도 적용
    rigidBody.Velocity += rigidBody.Acceleration * deltaTime;
    rigidBody.AngularVelocity += rigidBody.AngularAcceleration * deltaTime;
    
    // 중력 적용
    if (rigidBody.UseGravity) {
        const auto& gravity = GetComponent<GravityComponent>(entity);
        rigidBody.Velocity += gravity.Force * deltaTime;
    }
}

void FinalUpdate() override {
    // 속도 → 위치 적용
    transform.Position += rigidBody.Velocity * deltaTime;
    transform.Rotation += rigidBody.AngularVelocity * deltaTime;
}
```

#### 지원하는 물리 컴포넌트
```cpp
struct RigidBodyComponent {
    Vector3 Velocity;           // 선형 속도
    Vector3 Acceleration;       // 선형 가속도
    Vector3 AngularVelocity;    // 각속도
    Vector3 AngularAcceleration;// 각가속도
    float Mass;                 // 질량
    bool UseGravity;           // 중력 사용 여부
};

struct GravityComponent {
    Vector3 Force{0.0f, -9.81f, 0.0f}; // 중력 힘
};
```

### 2. PlayerControlSystem
**파일**: `PlayerControlSystem.h`
**역할**: 플레이어 입력을 게임 오브젝트 제어로 변환

#### 주요 기능
```cpp
- 키보드/게임패드 입력을 움직임으로 변환
- 플레이어 캐릭터 상태 관리
- 애니메이션 트리거 설정
- 카메라 연동
```

### 3. BoundingVolumeUpdateSystem
**파일**: `BoundingVolumeUpdateSystem.h`
**역할**: 충돌 검출용 바운딩 볼륨 업데이트

#### 주요 기능
```cpp
- 바운딩 박스/스피어 계산
- 월드 변환 적용
- 프러스텀 컬링 지원
- 충돌 검출 최적화
```

### 4. WorldMatrixUpdateSystem
**파일**: `WorldMatrixUpdateSystem.h`
**역할**: Transform 컴포넌트에서 월드 행렬 계산

#### 주요 기능
```cpp
- Position, Rotation, Scale을 월드 행렬로 변환
- 계층 구조 지원 (부모-자식 관계)
- 행렬 캐싱 최적화
- GPU 업로드 준비
```

---

## 리소스 관리 시스템

### 1. MeshSystem
**파일**: `MeshSystem.h`
**역할**: 메시 리소스 로딩 및 관리

#### 주요 기능
```cpp
- FBX, OBJ 등 3D 모델 파일 로딩
- 메시 데이터 최적화
- LOD(Level of Detail) 지원
- 메시 캐싱 및 재사용
```

### 2. MaterialSystem
**파일**: `MaterialSystem.h`
**역할**: 머티리얼 및 셰이더 파라미터 관리

#### 주요 기능
```cpp
- PBR(Physically Based Rendering) 머티리얼 지원
- 텍스처 바인딩 관리
- 셰이더 파라미터 설정
- 머티리얼 에디팅 지원
```

### 3. TextureSystem
**파일**: `TextureSystem.h`
**역할**: 텍스처 리소스 로딩 및 관리

#### 주요 기능
```cpp
- 다양한 이미지 포맷 지원 (PNG, JPG, DDS, etc.)
- 밉맵 생성 및 관리
- 텍스처 압축 지원
- GPU 메모리 최적화
```

### 4. InstanceSystem
**파일**: `InstanceSystem.h`
**역할**: 인스턴스 렌더링 관리

#### 주요 기능
```cpp
- 같은 메시의 다중 인스턴스 렌더링
- 인스턴스 데이터 버퍼 관리
- GPU 인스턴싱 최적화
- 동적 인스턴스 추가/제거
```

---

## 오디오 시스템

### 1. FMODAudioSystem
**파일**: `FMODAudioSystem.h`
**역할**: FMOD를 사용한 오디오 재생 및 관리

#### 주요 기능
```cpp
FMODAudioSystem() {
    // FMOD 시스템 초기화
    FMOD::System_Create(&sSystem);
    sSystem->init(512, FMOD_INIT_NORMAL, nullptr);
    FMODAudioRepository::Init(sSystem);
}

void Update() override {
    sSystem->update();  // FMOD 업데이트
    
    // 각 엔티티의 오디오 컴포넌트 처리
    for (ECS::Entity entity : mEntities) {
        auto& component = GetComponent<FMODAudioComponent>(entity);
        FMODAudioRepository::Play(component.handle, component.volume);
    }
}
```

#### 지원 기능
```cpp
- 3D 위치 기반 오디오
- 다중 채널 사운드
- 실시간 볼륨/피치 조절
- 오디오 스트리밍
- 사운드 이펙트 체인
```

#### 오디오 컴포넌트
```cpp
struct FMODAudioComponent {
    RepoHandle handle;  // 오디오 파일 핸들
    float volume;       // 볼륨 (0.0 ~ 1.0)
    float pitch;        // 피치 조절
    bool loop;          // 반복 재생
    bool is3D;          // 3D 사운드 여부
};
```

---

## 개발자 도구 시스템

### 1. ImGuiSystem
**파일**: `ImGuiSystem.h`
**역할**: 개발자 도구 UI 제공

#### 주요 기능
```cpp
- 실시간 시스템 모니터링
- 컴포넌트 에디터
- 성능 프로파일러
- 씬 하이어라키 뷰어
- 리소스 브라우저
```

#### 렌더링 통합
```cpp
void Render() {
    // ImGui 프레임 시작
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    
    // UI 요소들 렌더링
    RenderSystemMonitor();
    RenderComponentEditor();
    RenderSceneHierarchy();
    
    // ImGui 렌더링 완료
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

void RenderMultiViewport() {
    // 멀티 뷰포트 지원
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
}
```

### 2. TimeSystem
**파일**: `TimeSystem.h`
**역할**: 시간 관리 및 프레임레이트 제어

#### 주요 기능
```cpp
- 델타 타임 계산
- FPS 측정 및 제한
- 게임 시간 vs 실제 시간 관리
- 시간 스케일링 (슬로우 모션 등)
- 타이머 및 스케줄링
```

#### 시간 컴포넌트
```cpp
struct TimeComponent {
    float deltaTime;        // 프레임 간 시간
    float totalTime;        // 총 경과 시간
    float timeScale;        // 시간 배율
    uint32_t frameCount;    // 프레임 카운터
    float fps;              // 초당 프레임 수
};
```

### 3. LightSystem
**파일**: `LightSystem.h`
**역할**: 조명 시스템 관리

#### 주요 기능
```cpp
- 방향광(Directional Light) 지원
- 점광원(Point Light) 지원
- 스포트라이트(Spot Light) 지원
- 그림자 매핑
- 라이트 컬링 최적화
```

#### 조명 타입
```cpp
enum class LightType {
    Directional,  // 방향광 (태양광)
    Point,        // 점광원
    Spot,         // 스포트라이트
    Area          // 면광원
};

struct LightComponent {
    LightType type;
    Vector3 color;
    float intensity;
    Vector3 direction;  // 방향광/스포트라이트용
    float range;        // 점광원/스포트라이트용
    float spotAngle;    // 스포트라이트용
    bool castShadows;   // 그림자 생성 여부
};
```

---

## 시스템 간 의존성 그래프

```
                    ECSCoordinator (중앙 관리)
                           │
        ┌─────────────────┼─────────────────┐
        │                 │                 │
   WindowSystem     InputSystem      TimeSystem
        │                 │                 │
        └─────┬───────────┼─────────────────┘
              │           │
         DX12_RenderSystem ←─── CameraSystem
              │                      │
    ┌─────────┼─────────────────────┐│
    │         │                     ││
DX12_Device DX12_Command      DX12_SwapChain
    │         │                     │
DX12_Shader DX12_PSO         DX12_FrameResource
    │         │                     │
DX12_RootSig DX12_Mesh       DX12_SceneSystem
                                    │
              ┌─────────────────────┼─────────────────┐
              │                     │                 │
        PhysicsSystem         MaterialSystem   FMODAudioSystem
              │                     │                 │
    WorldMatrixUpdate        TextureSystem      ImGuiSystem
              │                     │                 │
      PlayerControl            InstanceSystem    LightSystem
```

---

## 성능 최적화 가이드

### 1. 시스템 순서 최적화
```cpp
// 의존성을 고려한 최적 실행 순서
BeginPlay    : 모든 시스템 초기화
Sync         : WindowSystem (메인 스레드), Input 동기화
PreUpdate    : 입력 처리, 시간 업데이트
Update       : 게임 로직, 물리 시뮬레이션
LateUpdate   : Transform 업데이트, 바운딩 볼륨 갱신
FixedUpdate  : 물리 적분, 네트워크 동기화
FinalUpdate  : 렌더링 데이터 준비, 오디오 업데이트
```

### 2. 메모리 최적화
```cpp
- Component Array 사용으로 캐시 친화적 메모리 레이아웃
- Repository 패턴으로 리소스 중복 방지
- 객체 풀링으로 동적 할당 최소화
- GPU 리소스 재사용 및 배치 최적화
```

### 3. 병렬 처리 최적화
```cpp
- 시스템별 독립적 병렬 실행
- 데이터 레이스 방지를 위한 뮤텍스 사용
- GPU와 CPU 작업 파이프라이닝
- 프레임 리소스 다중 버퍼링
```

---

## 확장 가이드

### 새로운 시스템 추가하기

1. **시스템 클래스 생성**
```cpp
class MyCustomSystem : public ECS::ISystem {
public:
    void BeginPlay() override {
        // 초기화 로직
    }
    
    void Update() override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        for (ECS::Entity entity : mEntities) {
            // 엔티티별 처리 로직
            auto& component = coordinator.GetComponent<MyComponent>(entity);
            // 컴포넌트 처리
        }
    }
};
```

2. **시스템 등록 및 시그니처 설정**
```cpp
void RegisterMySystem() {
    auto mySystem = gCoordinator.RegisterSystem<MyCustomSystem>();
    
    ECS::Signature signature;
    signature.set(gCoordinator.GetComponentType<MyComponent>());
    signature.set(gCoordinator.GetComponentType<TransformComponent>());
    
    gCoordinator.SetSystemSignature<MyCustomSystem>(signature);
}
```

3. **생명주기 통합**
```cpp
// 원하는 생명주기 단계에서 호출되도록 구현
// Update(), LateUpdate(), FinalUpdate() 등 오버라이드
```

### 플랫폼 포팅 가이드

1. **DirectX11 포팅**
```cpp
// DX11_Config.h 구현
// DX11_*System.h 파일들 구현
// 인터페이스는 DX12와 동일하게 유지
```

2. **Vulkan 포팅**
```cpp
// Vulkan_Config.h 구현
// VK_*System.h 파일들 구현
// 멀티스레드 커맨드 버퍼 활용 가능
```

---

이 가이드를 통해 각 시스템의 역할과 사용법을 이해하고, 필요에 따라 새로운 시스템을 추가하거나 기존 시스템을 확장할 수 있습니다.
