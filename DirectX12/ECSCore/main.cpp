#include "ECSCoordinator.h"

#include "PhysicsSystem.h"

void InitExample()
{
    auto& coordinator = ECS::Coordinator::GetInstance();

    // 1. Register Components
    coordinator.RegisterComponent<Transform>();
    coordinator.RegisterComponent<RigidBody>();
    coordinator.RegisterComponent<Gravity>();

    // 2. Register System
    auto physicsSystem = coordinator.RegisterSystem<PhysicsSystem>();

    ECS::Signature signature;
    signature.set(coordinator.GetComponentType<Transform>());
    signature.set(coordinator.GetComponentType<RigidBody>());
    signature.set(coordinator.GetComponentType<Gravity>());
    coordinator.SetSystemSignature<PhysicsSystem>(signature);

    // 3. Create Entity
    ECS::Entity ball = coordinator.CreateEntity();
    coordinator.AddComponent(ball, Transform{});
    coordinator.AddComponent(ball, RigidBody{});
    coordinator.AddComponent(ball, Gravity{});

    ECS::Entity ball2 = coordinator.CreateEntity();
    coordinator.AddComponent(ball2, Transform{});
    coordinator.AddComponent(ball2, RigidBody{});
    coordinator.AddComponent(ball2, Gravity{ DirectX::SimpleMath::Vector3(1.0f, 0.0f, 1.0f)});

    // 4. Simulation loop
    for (int i = 0; i < 10; ++i) {
        physicsSystem->Update(0.016f); // 16ms timestep

        const auto& tf = coordinator.GetComponent<Transform>(ball);
        std::cout << "Position at step " << i << ": ("
            << tf.position.x << ", "
            << tf.position.y << ", "
            << tf.position.z << ")\n";

        const auto& tf2 = coordinator.GetComponent<Transform>(ball2);
        std::cout << "Position at step " << i << ": ("
            << tf2.position.x << ", "
            << tf2.position.y << ", "
            << tf2.position.z << ")\n";
    }
}

int main()
{
	ECS::Coordinator::GetInstance().Init();

    InitExample();

	return 0;
}