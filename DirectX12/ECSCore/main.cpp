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

//void InitMeshExample()
//{
//    auto& coordinator = ECS::Coordinator::GetInstance();
//
//    // 1. Register Components
//    coordinator.RegisterComponent<MeshComponent>();
//
//    // 2. Register System
//    coordinator.RegisterSystem<MeshSystem>();
//
//    ECS::Signature signature;
//    signature.set(coordinator.GetComponentType<MeshComponent>());
//    coordinator.SetSystemSignature<MeshSystem>(signature);
//
//    // 3. Create Entity
//    ECS::Entity mesh1 = coordinator.CreateEntity();
//    ECS::Entity mesh2 = coordinator.CreateEntity();
//    ECS::Entity mesh3 = coordinator.CreateEntity();
//
//    std::string path = GetSoundLocation();
//    ECS::RepoHandle handle = MeshRepository::GetInstance().Load(path + "jaguar");
//    coordinator.AddComponent(mesh1, MeshComponent{ handle });
//    handle = MeshRepository::GetInstance().Load(path + "singing");
//    coordinator.AddComponent(mesh2, MeshComponent{ handle });
//    handle = MeshRepository::GetInstance().Load();
//    coordinator.AddComponent(mesh3, MeshComponent{ handle });
//}

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

void InitDX12_TransformExample()
{
    auto& coordinator = ECS::Coordinator::GetInstance();

    // 1. Register Components
    coordinator.RegisterComponent<DX12_TransformComponent>();
    coordinator.RegisterComponent<DX12_BoundingComponent>();
    coordinator.RegisterComponent<DX12_MeshComponent>();
    coordinator.RegisterComponent<InstanceData>();

    // 2. Register System
    coordinator.RegisterSystem<DX12_TransformSystem>();
    coordinator.RegisterSystem<DX12_BoundingSystem>();
    coordinator.RegisterSystem<DX12_InstanceSystem>();

    ECS::Signature signature;
    signature.set(coordinator.GetComponentType<DX12_TransformComponent>());
    coordinator.SetSystemSignature<DX12_TransformSystem>(signature);

    signature.set(coordinator.GetComponentType<DX12_MeshComponent>());
    signature.set(coordinator.GetComponentType<DX12_BoundingComponent>());
    coordinator.SetSystemSignature<DX12_BoundingSystem>(signature);

    ECS::Signature signatureInstance;
	signatureInstance.set(coordinator.GetComponentType<DX12_TransformComponent>());
	signatureInstance.set(coordinator.GetComponentType<InstanceData>());
    coordinator.SetSystemSignature<DX12_InstanceSystem>(signatureInstance);

    // 3. Create Entity
    ECS::Entity entity1 = coordinator.CreateEntity();
    ECS::Entity entity2 = coordinator.CreateEntity();
    ECS::Entity entity3 = coordinator.CreateEntity();

    coordinator.AddComponent(entity1, DX12_TransformComponent{ });
    coordinator.AddComponent(entity1, DX12_BoundingComponent{ });
    coordinator.AddComponent(entity1, DX12_MeshComponent{ });
    coordinator.AddComponent(entity1, InstanceData{ });
    coordinator.AddComponent(entity2, DX12_TransformComponent{ });
    coordinator.AddComponent(entity2, DX12_BoundingComponent{ });
    coordinator.AddComponent(entity2, DX12_MeshComponent{ });
    coordinator.AddComponent(entity2, InstanceData{ });
    coordinator.AddComponent(entity3, DX12_TransformComponent{ });
    coordinator.AddComponent(entity3, DX12_BoundingComponent{ });
    coordinator.AddComponent(entity3, DX12_MeshComponent{ });
    coordinator.AddComponent(entity3, InstanceData{ });
}

void LoadScene() {
    // 팩토리를 통해 게임 오브젝트를 간단하게 생성
    auto ball1 = GameObjectFactory::GetInstance().CreateGameObject("BouncingBall");
    auto ball2 = GameObjectFactory::GetInstance().CreateGameObject("BouncingBall");

    auto& ball1Transform = ECS::Coordinator::GetInstance().GetComponent<DX12_TransformComponent>(ball1);
    ball1Transform.w_Position = { -5.f, 10.f, 0.f };
}

void RunExample()
{
    //================================
	// Load Coordinator Instance
    //================================
    ECS::Coordinator::GetInstance().Init();

    LOG_INFO("ECS Coordinator Init {} {} {} {}", 1,2,3,4);
    LOG_WARN("ECS Coordinator Init");
	LOG_ERROR("ECS Coordinator Init");
	LOG_FATAL("ECS Coordinator Init");

    ECS::Coordinator::GetInstance().RegisterSystem<TimeSystem>();
    ECS::Coordinator::GetInstance().RegisterSystem<WindowSystem>();
    ECS::Coordinator::GetInstance().RegisterSystem<DX12_RenderSystem>();
    InitPhysicsExample();
    InitSoundExample();
    // InitMeshExample();
    InitLightExample();
    InitDX12_TransformExample();

    //================================
    // Part 3. Mesh Example
    //================================
    LoadScene();

    //================================
    // Simulation loop
    //================================
    ECS::Coordinator::GetInstance().Run();
}

int main()
{
    RunExample();

	return 0;
}
