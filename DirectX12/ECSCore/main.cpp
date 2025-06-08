#include "pch.h"

std::string GetSoundLocation() {

    char path[MAX_PATH] = { 0 };
#ifdef _WIN32
    if (GetModuleFileNameA(nullptr, path, sizeof(path)) == 0)
        return std::string();
#else // _WIN32
#error Unsupported platform for GetSlInterposerDllLocation!
#endif // _WIN32

    auto basePath = std::filesystem::path(path).parent_path().parent_path().parent_path();
    // auto dllPath = basePath.wstring().append(L"\\StreamlineCore\\_bin\\sl.interposer.dll");
    // auto dllPath = basePath.wstring().append(L"\\Libraries\\Include\\DirectX\\Streamline\\bin\\x64\\sl.interposer.dll");
    // auto dllPath = basePath.wstring().append(L"\\Libraries\\Include\\DirectX\\Streamline2\\bin\\x64\\sl.interposer.dll");
    auto dllPath = basePath.string().append("\\Data\\Sound\\");
    return dllPath;
}

void InitPhysicsExample()
{
    auto& coordinator = ECS::Coordinator::GetInstance();

    // 1. Register Components
    coordinator.RegisterComponent<TransformComponent>();
    coordinator.RegisterComponent<RigidBodyComponent>();
    coordinator.RegisterComponent<GravityComponent>();

    // 2. Register System
    coordinator.RegisterSystem<PhysicsSystem>();

    ECS::Signature signature;
    signature.set(coordinator.GetComponentType<TransformComponent>());
    signature.set(coordinator.GetComponentType<RigidBodyComponent>());
    signature.set(coordinator.GetComponentType<GravityComponent>());
    coordinator.SetSystemSignature<PhysicsSystem>(signature);

    // 3. Create Entity
    ECS::Entity ball = coordinator.CreateEntity();
    coordinator.AddComponent(ball, TransformComponent{});
    coordinator.AddComponent(ball, RigidBodyComponent{});
    coordinator.AddComponent(ball, GravityComponent{});

    ECS::Entity ball2 = coordinator.CreateEntity();
    coordinator.AddComponent(ball2, TransformComponent{});
    coordinator.AddComponent(ball2, RigidBodyComponent{});
    coordinator.AddComponent(ball2, GravityComponent{ DirectX::SimpleMath::Vector3(1.0f, 0.0f, 1.0f) });
}

void InitSoundExample()
{
    auto& coordinator = ECS::Coordinator::GetInstance();

    // 1. Register Components
    coordinator.RegisterComponent<FMODAudioComponent>();

    // 2. Register System
    coordinator.RegisterSystem<FMODAudioSystem>();

    ECS::Signature signature;
    signature.set(coordinator.GetComponentType<FMODAudioComponent>());
    coordinator.SetSystemSignature<FMODAudioSystem>(signature);

    // 3. Create Entity
    ECS::Entity sound1 = coordinator.CreateEntity();
    ECS::Entity sound2 = coordinator.CreateEntity();
    ECS::Entity sound3 = coordinator.CreateEntity();

    std::string path = GetSoundLocation();
    AudioHandle handle = FMODAudioRepository::LoadSound("jaguar", path + "jaguar.wav", false);
    coordinator.AddComponent(sound1, FMODAudioComponent{ handle, 0.5f, true });
    handle = FMODAudioRepository::LoadSound("singing", path + "singing.wav", false);
    coordinator.AddComponent(sound2, FMODAudioComponent{ handle, 0.5f, true });
    handle = FMODAudioRepository::LoadSound("swish", path + "swish.wav", false);
    coordinator.AddComponent(sound3, FMODAudioComponent{ handle, 0.5f, true });
}

void InitMeshExample()
{
    auto& coordinator = ECS::Coordinator::GetInstance();

    // 1. Register Components
    coordinator.RegisterComponent<MeshComponent>();

    // 2. Register System
    coordinator.RegisterSystem<MeshSystem>();

    ECS::Signature signature;
    signature.set(coordinator.GetComponentType<MeshComponent>());
    coordinator.SetSystemSignature<MeshSystem>(signature);

    // 3. Create Entity
    ECS::Entity mesh1 = coordinator.CreateEntity();
    ECS::Entity mesh2 = coordinator.CreateEntity();
    ECS::Entity mesh3 = coordinator.CreateEntity();

    std::string path = GetSoundLocation();
    ECS::RepoHandle handle = MeshRepository::GetInstance().Load(path + "jaguar");
    coordinator.AddComponent(mesh1, MeshComponent{ handle });
    handle = MeshRepository::GetInstance().Load(path + "singing");
    coordinator.AddComponent(mesh2, MeshComponent{ handle });
    handle = MeshRepository::GetInstance().Load();
    coordinator.AddComponent(mesh3, MeshComponent{ handle });
}

void InitLightExample()
{
    auto& coordinator = ECS::Coordinator::GetInstance();

    // 1. Register Components
    coordinator.RegisterComponent<LightComponent>();

    // 2. Register System
    coordinator.RegisterSystem<LightSystem>();

    ECS::Signature signature;
    signature.set(coordinator.GetComponentType<LightComponent>());
    coordinator.SetSystemSignature<LightSystem>(signature);

    // 3. Create Entity
    ECS::Entity light1 = coordinator.CreateEntity();
    ECS::Entity light2 = coordinator.CreateEntity();
    ECS::Entity light3 = coordinator.CreateEntity();

    coordinator.AddComponent(light1, LightComponent{ });
    coordinator.AddComponent(light2, LightComponent{ });
    coordinator.AddComponent(light3, LightComponent{ });
}

void InitDX12()
{
    DX12_Core::GetInstance().Initialize();
    DX12_DescriptorHeapRepository::GetInstance().Initialize();

    auto& coordinator = ECS::Coordinator::GetInstance();

    // 1. Register Components
    coordinator.RegisterComponent<RenderTargetComponent>();
}

void InitExample()
{
    //================================
	// Load Coordinator Instance
    //================================
    LOG_INFO("ECS Coordinator Init {} {} {} {}", 1,2,3,4);
    LOG_WARN("ECS Coordinator Init");
	LOG_ERROR("ECS Coordinator Init");
	LOG_FATAL("ECS Coordinator Init");

	
    InitDX12();
    ECS::Coordinator::GetInstance().RegisterSystem<TimeSystem>();
    ECS::Coordinator::GetInstance().RegisterSystem<WindowSystem>();
    InitPhysicsExample();
    InitSoundExample();
    InitMeshExample();
    InitLightExample();

    //================================
    // Part 3. Mesh Example
    //================================

    //================================
    // Simulation loop
    //================================
    ECS::Coordinator::GetInstance().Run();
}

int main()
{
	ECS::Coordinator::GetInstance().Init();

    InitExample();

	return 0;
}