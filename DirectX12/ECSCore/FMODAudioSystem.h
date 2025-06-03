#pragma once
#include "ECSCoordinator.h"
#include "FMODAudioRepository.h"

class FMODAudioSystem : public ECS::ISystem {
public:
    FMODAudioSystem()
    {
		if (sSystem != nullptr) {
			std::cerr << "FMOD System already initialized." << std::endl;
			return;
		}

        FMOD_RESULT result = FMOD::System_Create(&sSystem);
		if (result != FMOD_OK) {
			std::cerr << "FMOD System creation failed: " << std::endl;
		}
        result = sSystem->init(512, FMOD_INIT_NORMAL, nullptr);
        if (result != FMOD_OK) {
            std::cerr << "FMOD System initialize failed: " << std::endl;
        }
        FMODAudioRepository::Init(sSystem);
    }
    ~FMODAudioSystem()
    {
		FMODAudioRepository::Shutdown();
		sSystem->release();
    }
    
    void Update(float dt) override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        sSystem->update();
        for (ECS::Entity entity : mEntities) {
            auto& component = coordinator.GetComponent<FMODAudioComponent>(entity);
            FMODAudioRepository::Play(component.handle, component.volume);
        }
    }

private:
    static inline FMOD::System* sSystem = nullptr;
};