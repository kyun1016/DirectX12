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
    // coordinator.AddComponent(sound1, FMODAudioComponent{ path + "jaguar.wav", "jaguar", 0.5f, true, true, false });
    // coordinator.AddComponent(sound2, FMODAudioComponent{ path + "singing.wav", "singing", 0.5f, true, true, false });
    // coordinator.AddComponent(sound3, FMODAudioComponent{ path + "swish.wav", "swish", 0.5f, true, true, false });
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
    coordinator.AddComponent(mesh1, MeshComponent{ 0 });
    coordinator.AddComponent(mesh2, MeshComponent{ 0 });
    coordinator.AddComponent(mesh3, MeshComponent{ 0 });
}

void InitExample()
{
    //================================
	// Load Coordinator Instance
    //================================
    auto& coordinator = ECS::Coordinator::GetInstance();

    InitPhysicsExample();
    InitSoundExample();
    InitMeshExample();

    //================================
    // Part 3. Mesh Example
    //================================

    //================================
    // Simulation loop
    //================================
    
    for (int i = 0; i < 10000; ++i) {
        coordinator.UpdateAllSystem(0.16f);

        //const auto& tf = coordinator.GetComponent<TransformComponent>(ball);
        //std::cout << "Position at step " << i << ": ("
        //    << tf.position.x << ", "
        //    << tf.position.y << ", "
        //    << tf.position.z << ")\n";

        //const auto& tf2 = coordinator.GetComponent<TransformComponent>(ball2);
        //std::cout << "Position at step " << i << ": ("
        //    << tf2.position.x << ", "
        //    << tf2.position.y << ", "
        //    << tf2.position.z << ")\n";
    }
}

int main()
{
	ECS::Coordinator::GetInstance().Init();

    InitExample();

	return 0;
}