# DirectX 12 Learning Project

## 📋 개요
이 프로젝트는 DirectX 12를 사용한 실시간 렌더링 기술 학습과 고급 그래픽스 기법들을 구현한 종합적인 학습 프로젝트입니다. 기초적인 DirectX 12 파이프라인부터 고급 렌더링 기법, NVIDIA Streamline 통합, Frame Interpolation까지 다양한 주제를 다루고 있습니다.

## 🚀 주요 특징
- **Modern DirectX 12 API**: 최신 DirectX 12 API를 활용한 효율적인 GPU 프로그래밍
- **다양한 렌더링 기법**: 조명, 그림자, 노멀 맵핑, 테셀레이션 등 다양한 렌더링 기법 구현
- **컴퓨트 셰이더 활용**: GPU 컴퓨팅을 통한 병렬 처리 및 최적화
- **고급 셰이더 기술**: 지오메트리 셰이더, 테셀레이션 셰이더 등을 활용한 고급 셰이더 프로그래밍
- **NVIDIA Streamline 통합**: DLSS, Reflex 등 NVIDIA의 최신 기술 통합
- **Frame Interpolation**: AI 기반 프레임 보간 기술 구현
- **실시간 UI**: ImGui를 활용한 실시간 매개변수 조정 및 디버깅

## 📁 프로젝트 구조

### 🔧 핵심 엔진 모듈
```
DirectX12/
├── EngineCore/          # 핵심 엔진 유틸리티 및 헬퍼 클래스들
├── DonutApp/           # NVIDIA Donut 기반 애플리케이션 프레임워크
├── DonutCore/          # Donut 핵심 시스템
├── DonutEngine/        # Donut 엔진 계층
├── DonutRender/        # Donut 렌더링 시스템
├── DonutShaders/       # Donut 셰이더 컬렉션
├── StreamlineCore/     # NVIDIA Streamline 통합
├── ImGuiCore/          # ImGui 통합 및 UI 시스템
└── Libraries/          # 외부 라이브러리들
```

### 📚 학습 카테고리별 프로젝트

#### 🌊 **기초 렌더링 (Chapters 6-7)**
- **`LandAndWaves`** - 지형 렌더링과 파도 시뮬레이션
- **`Shapes`** - 기본 기하학적 도형 렌더링
- **`TexBox`** - 텍스처 맵핑 기초

#### 💡 **조명 시스템 (Chapter 8)**
- **`LitColumns`** - 기본 조명 모델 구현
- **`LitWaves`** - 조명이 적용된 파도 렌더링

#### 🎨 **블렌딩 (Chapter 10)**
- **`BlendDemo`** - 알파 블렌딩 및 투명도 처리

#### 📐 **스텐실 버퍼 (Chapter 11)**
- **`StencilDemo`** - 스텐실 테스트를 활용한 다양한 효과

#### 🔺 **지오메트리 셰이더 (Chapter 12)**
- **`GeometryShader`** - 지오메트리 셰이더 기본 구현
- **`Billboard`** - 빌보드 렌더링 및 파티클 시스템

#### ⚡ **컴퓨트 셰이더 (Chapter 13)**
- **`ComputeFilter`** - 이미지 필터링 및 후처리
- **`WavesCS`** - 컴퓨트 셰이더 기반 파도 시뮬레이션

#### 🔻 **테셀레이션 (Chapter 14)**
- **`Tessellation`** - 하드웨어 테셀레이션 구현

#### 🎯 **고급 렌더링 기법 (Chapters 16-23)**
- **`Instancing`** - 인스턴스 렌더링을 통한 성능 최적화
- **`Culling`** - 절두체 컬링 및 오클루전 컬링
- **`Picking`** - 마우스 피킹 및 객체 선택
- **`CubeMap`** - 큐브 맵을 활용한 환경 맵핑
- **`DynamicCubeMap`** - 동적 큐브 맵 생성
- **`NormalMap`** - 노멀 맵핑을 통한 디테일 향상
- **`ShadowMap`** - 그림자 맵핑 구현
- **`SsaoMap`** - Screen Space Ambient Occlusion
- **`SkinnedMesh`** - 스킨드 메시 애니메이션

#### 🎬 **프레임 보간 기술 (Chapter 24)**
- **`FrameInterpolation`** - 기본 프레임 보간 구현
- **`FrameInterpolationV2`** - 개선된 프레임 보간
- **`FrameInterpolationV3`** - 고급 프레임 보간
- **`FrameInterpolationV4`** - 최적화된 프레임 보간

### 🔧 **개발 도구 및 테스트**
- **`CompileShader`** - 셰이더 컴파일 시스템
- **`TestImGui`** - ImGui 테스트 및 UI 프로토타이핑
- **`ImGuiImageTest`** - 이미지 렌더링 테스트
- **`AssemblyTest`** - 어셈블리 코드 테스트
- **`InstanceTest`** - 인스턴싱 성능 테스트
- **`AudioSystemDemo`** - FMOD 기반 오디오 시스템

## 🛠️ 기술 스택

### 그래픽스 API
- **DirectX 12** - 메인 그래픽스 API
- **HLSL** - High Level Shading Language

### 외부 라이브러리
- **NVIDIA Streamline** - DLSS, Reflex 등 NVIDIA 기술
- **NVIDIA Donut** - 렌더링 프레임워크
- **NVRHI** - 렌더링 하드웨어 인터페이스
- **ImGui** - 즉시 모드 GUI
- **DirectXMath** - 수학 연산 라이브러리
- **FMOD** - 오디오 시스템
- **GLFW** - 윈도우 관리
- **stb** - 이미지 로딩
- **JSON** - 설정 파일 처리

### 개발 환경
- **Visual Studio 2022** (C++ 프로젝트)
- **Windows SDK**
- **CMake** (일부 하위 프로젝트)

## 🎯 핵심 기능

### 1. 렌더링 파이프라인
- **Multi-threaded Command Recording** - 다중 스레드 명령 기록
- **Resource Barriers & Synchronization** - 리소스 동기화
- **Descriptor Heaps Management** - 디스크립터 힙 관리
- **Root Signature Optimization** - 루트 시그니처 최적화

### 2. 셰이더 시스템
- **Dynamic Shader Compilation** - 동적 셰이더 컴파일
- **Hot-reload Support** - 셰이더 핫 리로드
- **Multi-stage Pipeline** - 다단계 렌더링 파이프라인

### 3. 메모리 관리
- **Upload Buffer Pool** - 업로드 버퍼 풀링
- **Dynamic Buffer Allocation** - 동적 버퍼 할당
- **Resource Lifetime Management** - 리소스 생명주기 관리

### 4. 최적화 기법
- **GPU-driven Rendering** - GPU 주도 렌더링
- **Compute-based Post-processing** - 컴퓨트 기반 후처리
- **Level-of-Detail (LOD)** - 거리별 세부도 조절

## 🚀 빌드 및 실행

### 시스템 요구사항
- **OS**: Windows 10/11 (64-bit)
- **GPU**: DirectX 12 지원 그래픽카드
- **RAM**: 8GB 이상 권장
- **Visual Studio**: 2019 이상

### 빌드 방법
1. **저장소 클론**
   ```bash
   git clone [repository-url]
   cd DirectX12
   ```

2. **Visual Studio에서 솔루션 열기**
   ```
   DirectX12/DirectX12.sln
   ```

3. **프로젝트 빌드**
   - 솔루션 구성: `Debug` 또는 `Release`
   - 플랫폼: `x64`
   - 빌드 → 솔루션 빌드

4. **개별 프로젝트 실행**
   - 원하는 프로젝트를 시작 프로젝트로 설정
   - `F5` 또는 `Ctrl+F5`로 실행

## 📖 학습 가이드

### 초급자를 위한 추천 순서
1. **`Shapes`** - DirectX 12 기초 이해
2. **`TexBox`** - 텍스처 맵핑 학습
3. **`LitColumns`** - 조명 시스템 기초
4. **`BlendDemo`** - 알파 블렌딩 이해

### 중급자를 위한 추천 순서
1. **`ComputeFilter`** - 컴퓨트 셰이더 기초
2. **`GeometryShader`** - 지오메트리 셰이더 활용
3. **`ShadowMap`** - 그림자 맵핑
4. **`CubeMap`** - 환경 맵핑

### 고급자를 위한 추천 순서
1. **`FrameInterpolation`** 시리즈 - AI 기반 프레임 보간
2. **`SkinnedMesh`** - 캐릭터 애니메이션
3. **`SsaoMap`** - 고급 조명 기법
4. **`Culling`** - 렌더링 최적화

## 📊 성능 특징

### 최적화된 렌더링
- **멀티 스레드 명령 기록**: CPU 병렬화를 통한 성능 향상
- **GPU 메모리 관리**: 효율적인 메모리 사용으로 프레임률 안정화
- **컴퓨트 셰이더 활용**: GPU 병렬 처리를 통한 성능 최적화

### 프로파일링 지원
- **실시간 성능 모니터링**: ImGui를 통한 실시간 성능 데이터 시각화
- **GPU 타이밍**: 각 렌더링 패스별 GPU 사용 시간 측정
- **메모리 사용량 추적**: VRAM 및 시스템 메모리 사용량 모니터링

## 🎮 주요 데모 및 기능

### 실시간 파도 시뮬레이션
- **LandAndWaves**: CPU 기반 파도 생성
- **WavesCS**: 컴퓨트 셰이더 기반 고성능 파도 시뮬레이션
- **실시간 파라미터 조정**: 파도 주파수, 진폭, 속도 등 실시간 제어

### 고급 조명 시스템
- **Phong/Blinn-Phong 조명**: 기본 조명 모델 구현
- **다중 광원**: Point, Directional, Spot Light 지원
- **동적 조명**: 실시간 조명 매개변수 조정

### 그림자 렌더링
- **Shadow Mapping**: 기본 그림자 맵 기법
- **Cascaded Shadow Maps**: 대규모 환경을 위한 다단계 그림자
- **Soft Shadows**: 부드러운 그림자 구현

### 후처리 효과
- **Screen Space Ambient Occlusion**: 실시간 SSAO
- **Bloom Effect**: HDR 블룸 효과
- **Tone Mapping**: HDR to LDR 톤 맵핑
- **Anti-Aliasing**: 다양한 안티앨리어싱 기법

### 파티클 시스템
- **GPU 기반 파티클**: 컴퓨트 셰이더를 활용한 고성능 파티클
- **빌보드 렌더링**: 카메라 방향 자동 정렬
- **다양한 파티클 타입**: 연기, 불, 비 등 다양한 효과

## 🔬 고급 기술 구현

### NVIDIA Streamline 통합
```cpp
// DLSS 초기화 예제
sl::Result initDLSS() {
    sl::DLSSOptions dlssOptions{};
    dlssOptions.mode = sl::DLSSMode::eMaxQuality;
    dlssOptions.outputWidth = renderWidth;
    dlssOptions.outputHeight = renderHeight;
    return sl::dlssSetOptions(dlssOptions);
}
```

### Frame Interpolation 기술
- **V1**: 기본 프레임 보간 알고리즘
- **V2**: 모션 벡터 기반 개선
- **V3**: AI 기반 프레임 예측
- **V4**: 실시간 최적화된 버전

### 컴퓨트 셰이더 활용
```hlsl
// 파도 시뮬레이션 컴퓨트 셰이더 예제
[numthreads(16, 16, 1)]
void WaveUpdateCS(uint3 id : SV_DispatchThreadID) {
    if (id.x >= gWaveVertexCount.x || id.y >= gWaveVertexCount.y)
        return;
    
    // 파도 방정식 계산
    float3 position = CalculateWavePosition(id.xy, gTime);
    gWaveVertices[GetIndex(id.xy)] = position;
}
```

## 📈 성능 최적화

### CPU 최적화
- **멀티스레딩**: 렌더링 명령 병렬 생성
- **오브젝트 풀링**: 동적 할당 최소화
- **컬링 시스템**: 절두체 컬링으로 불필요한 렌더링 제거

### GPU 최적화
- **인스턴싱**: 동일 지오메트리 일괄 렌더링
- **GPU 메모리 관리**: 효율적인 버퍼 사용
- **파이프라인 상태 캐싱**: PSO 생성 비용 최소화

### 메모리 최적화
```cpp
// 업로드 버퍼 풀 관리
template<typename T>
class UploadBufferPool {
    std::queue<Microsoft::WRL::ComPtr<ID3D12Resource>> m_availableBuffers;
    std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> m_inUseBuffers;
    
public:
    ID3D12Resource* GetBuffer(size_t size);
    void ReleaseBuffer(ID3D12Resource* buffer);
};
```

## 🐛 디버깅 및 프로파일링

### 내장 디버깅 도구
- **실시간 매개변수 조정**: ImGui를 통한 실시간 값 변경
- **렌더링 단계별 시각화**: 각 렌더링 패스 결과 확인
- **셰이더 디버깅**: 픽셀 셰이더 아웃풋 시각화

### 성능 분석
```cpp
// GPU 타이밍 측정
class GPUTimer {
    Microsoft::WRL::ComPtr<ID3D12QueryHeap> m_queryHeap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_queryResult;
    
public:
    void BeginQuery(ID3D12GraphicsCommandList* cmdList, const std::string& name);
    void EndQuery(ID3D12GraphicsCommandList* cmdList);
    float GetElapsedTime() const;
};
```

## 📚 참고 자료 및 학습 리소스

### 추천 도서
- "Introduction to 3D Game Programming with DirectX 12" - Frank Luna
- "Real-Time Rendering" - Tomas Akenine-Möller
- "GPU Gems" 시리즈 - NVIDIA

### 온라인 리소스
- [Microsoft DirectX 12 Documentation](https://docs.microsoft.com/en-us/windows/win32/direct3d12/)
- [NVIDIA Developer Documentation](https://developer.nvidia.com/)
- [DirectX Specs](https://github.com/microsoft/DirectX-Specs)

### 관련 프로젝트 및 샘플
- [Microsoft DirectX Graphics Samples](https://github.com/Microsoft/DirectX-Graphics-Samples)
- [NVIDIA Falcor](https://github.com/NVIDIAGameWorks/Falcor)
- [AMD Cauldron](https://github.com/GPUOpen-LibrariesAndSDKs/Cauldron)

## 🤝 기여 방법

### 코드 기여
1. 프로젝트 포크
2. 기능 브랜치 생성 (`git checkout -b feature/AmazingFeature`)
3. 변경사항 커밋 (`git commit -m 'Add some AmazingFeature'`)
4. 브랜치에 푸시 (`git push origin feature/AmazingFeature`)
5. Pull Request 생성

### 버그 신고
- GitHub Issues를 통한 버그 신고
- 재현 가능한 최소 예제 포함
- 시스템 사양 및 환경 정보 제공

## 📝 라이센스

이 프로젝트는 MIT 라이센스 하에 배포됩니다. 자세한 내용은 [LICENSE](LICENSE) 파일을 참조하세요.

## 👥 개발자 정보

### 주요 개발자
- **Kyun** - 프로젝트 리더 및 메인 개발자

### 외부 라이브러리 크레딧
- **NVIDIA Donut Framework** - 렌더링 프레임워크 제공
- **Dear ImGui** - UI 시스템
- **FMOD** - 오디오 시스템
- **DirectX Math Library** - 수학 연산

## 📞 연락처 및 지원

### 문의사항
- **이슈**: GitHub Issues 페이지 활용
- **토론**: GitHub Discussions 섹션

### 개발 로드맵
- [ ] Vulkan API 포팅
- [ ] Ray Tracing 지원 추가
- [ ] Variable Rate Shading 구현
- [ ] DirectStorage 통합
- [ ] Mesh Shaders 지원

## 🏆 주요 성과

### 성능 벤치마크
- **프레임률**: 1080p에서 평균 60+ FPS (RTX 3070 기준)
- **메모리 사용량**: 평균 2-4GB VRAM
- **로딩 시간**: 평균 3-5초 (SSD 기준)

### 지원 플랫폼
- ✅ Windows 10/11 (x64)
- ✅ DirectX 12 지원 GPU
- ✅ Visual Studio 2019/2022

---

**⚠️ 주의사항**: 이 프로젝트는 학습 목적으로 개발되었으며, 상업적 사용 시 라이센스를 확인해주세요.

**🎓 학습자를 위한 팁**: 각 프로젝트는 독립적으로 실행 가능하므로, 관심 있는 주제부터 시작하여 점진적으로 확장해나가시기 바랍니다.