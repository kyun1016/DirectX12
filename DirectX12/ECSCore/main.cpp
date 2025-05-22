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

    // 4. Simulation loop
    for (int i = 0; i < 10; ++i) {
        physicsSystem->Update(0.016f); // 16ms timestep

        const auto& tf = coordinator.GetComponent<Transform>(ball);
        std::cout << "Position at step " << i << ": ("
            << tf.position.x << ", "
            << tf.position.y << ", "
            << tf.position.z << ")\n";
    }
}

int main()
{
	ECS::Coordinator::GetInstance().Init();

    InitExample();

	return 0;
}