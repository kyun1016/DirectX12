#pragma once
#include "ECSCoordinator.h"
#include "DX12_TransformComponent.h"
#include "DX12_MeshSystem.h"
#include "PhysicsComponent.h"

class GameObjectFactory {
DEFAULT_SINGLETON(GameObjectFactory)
public:

    // 미리 정의된 타입의 게임 오브젝트를 생성
    ECS::Entity CreateGameObject(const std::string& objectType)
    {
        auto& coordinator = ECS::Coordinator::GetInstance();

        // 타입에 맞는 생성 함수 호출
        //if (objectType == "Player") {
        //    return createPlayer();
        //}
        if (objectType == "BouncingBall") {
            return CreateBouncingBall();
        }
        // ...
        LOG_WARN("Unknown game object type requested: {}", objectType);
        return 0; // Invalid Entity
    }

private:
    ECS::Entity CreateBouncingBall() {
        auto& coordinator = ECS::Coordinator::GetInstance();

        // 1. 빈 엔티티 생성
        ECS::Entity ball = coordinator.CreateEntity();

        // 2. 필요한 컴포넌트들을 조합하여 추가
        coordinator.AddComponent(ball, DX12_TransformComponent{}); // 위치, 회전, 크기 데이터
        coordinator.AddComponent(ball, RigidBodyComponent{});      // 물리 속도 데이터
        coordinator.AddComponent(ball, GravityComponent{});      // 중력 데이터

        // 메쉬 컴포넌트를 추가하여 외형을 부여
        ECS::RepoHandle boxMeshHandle = DX12_MeshRepository::GetInstance().Load("Box");
        coordinator.AddComponent(ball, DX12_MeshComponent{ boxMeshHandle });

        LOG_INFO("Created a BouncingBall object with Entity ID: {}", ball);
        return ball;
    }
};