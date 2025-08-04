#pragma once
#include "ECSCoordinator.h"
#include "InstanceComponent.h"

class WorldMatrixUpdateSystem : public ECS::ISystem {
public:
	void RegisterComponent() override {
		ECS::Coordinator::GetInstance().RegisterComponent<TransformComponent>();
		ECS::Coordinator::GetInstance().RegisterComponent<DX12_MeshComponent>();
		ECS::Coordinator::GetInstance().RegisterComponent<CFGInstanceComponent>();
		ECS::Coordinator::GetInstance().RegisterComponent<TextureScaleComponent>();
	}
	void RegisterSignature() override {
		ECS::Signature signature;
		signature.set(ECS::Coordinator::GetInstance().GetComponentType<TransformComponent>());
		signature.set(ECS::Coordinator::GetInstance().GetComponentType<DX12_MeshComponent>());
		signature.set(ECS::Coordinator::GetInstance().GetComponentType<CFGInstanceComponent>());
		signature.set(ECS::Coordinator::GetInstance().GetComponentType<TextureScaleComponent>());
		ECS::Coordinator::GetInstance().SetSystemSignature<WorldMatrixUpdateSystem>(signature);
	}
	void Update() override {
		auto& coordinator = ECS::Coordinator::GetInstance();
		for (ECS::Entity entity : mEntities) {
			auto& transform = coordinator.GetComponent<TransformComponent>(entity);
			if (!transform.Dirty)
				continue;

			auto& boundingVolumn = coordinator.GetComponent<BoundingVolumnComponent>(entity);
			auto& mesh = coordinator.GetComponent<DX12_MeshComponent>(entity);
			auto& cfg = coordinator.GetComponent<CFGInstanceComponent>(entity);
			auto& textureScale = coordinator.GetComponent<TextureScaleComponent>(entity);
			auto& instanceData = coordinator.GetComponent<InstanceData>(entity);

			float rx = DirectX::XMConvertToRadians(transform.Rotation.x);
			float ry = DirectX::XMConvertToRadians(transform.Rotation.y);
			float rz = DirectX::XMConvertToRadians(transform.Rotation.z);
			transform.RotationQuat = DirectX::XMQuaternionRotationRollPitchYaw(rx, ry, rz);

			DirectX::XMMATRIX rotX = DirectX::XMMatrixRotationX(rx);
			DirectX::XMMATRIX rotY = DirectX::XMMatrixRotationY(ry);
			DirectX::XMMATRIX rotZ = DirectX::XMMatrixRotationZ(rz);

			DirectX::XMMATRIX rot = rotX * rotY * rotZ;
			if (cfg.Option & eCFGInstanceComponent::UseQuat)
				rot = DirectX::XMMatrixRotationQuaternion(transform.RotationQuat);

			DirectX::XMMATRIX world
				= DirectX::XMMatrixScaling(transform.Scale.x, transform.Scale.y, transform.Scale.z)
				* rot
				* DirectX::XMMatrixTranslation(transform.Position.x, transform.Position.y, transform.Position.z);
			DirectX::XMMATRIX texTransform = DirectX::XMMatrixScaling(textureScale.TextureScale.x, textureScale.TextureScale.y, textureScale.TextureScale.z);
			DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(world);

			instanceData.World = DirectX::XMMatrixTranspose(world);
			instanceData.TexTransform = DirectX::XMMatrixTranspose(texTransform);
			instanceData.WorldInvTranspose = DirectX::XMMatrixInverse(&det, world);
		}
	}
};