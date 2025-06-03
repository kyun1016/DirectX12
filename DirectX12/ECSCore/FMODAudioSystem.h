#pragma once
#include "ECSCoordinator.h"
#include "FMODAudioComponent.h"

class FMODAudioSystem : public ECS::ISystem {
public:
    FMODAudioSystem()
    {
        FMOD_RESULT result = FMOD::System_Create(&mSystem);
		if (result != FMOD_OK) {
			std::cerr << "FMOD System creation failed: " << std::endl;
		}
        result = mSystem->init(512, FMOD_INIT_NORMAL, nullptr);
        if (result != FMOD_OK) {
            std::cerr << "FMOD System creation failed: " << std::endl;
        }
    }
    ~FMODAudioSystem()
    {
        mSystem->release();
    }
    
    void Update(float dt) override {
        auto& coordinator = ECS::Coordinator::GetInstance();
        mSystem->update();
        for (ECS::Entity entity : mEntities) {
            auto& component = coordinator.GetComponent<FMODAudioComponent>(entity);
            if(component.playStart)
                PlaySound(component);
        }
    }

private:
    void CreateSound(FMODAudioComponent& component)
    {
        if (component.isLoaded) {
            return;
        }

        const int loopFlag = component.useLoop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
        FMOD_RESULT result = mSystem->createSound(component.filename.c_str(), loopFlag, 0, &mSoundMap[component.soundname]);

        if (result != FMOD_OK) {
            std::cout << "system->createSound() fail" << std::endl;
            exit(-1);
        }

		component.isLoaded = true;
    }

	void PlaySound(FMODAudioComponent& component)
	{
		if (!component.isLoaded) {
			CreateSound(component);
		}

        FMOD::Sound* sound = mSoundMap[component.soundname];
		FMOD::Channel* channel = mChannelMap[sound];
		FMOD_RESULT result = mSystem->playSound(sound, nullptr, false, &channel);
		if (result != FMOD_OK) {
			std::cout << "system->playSound() fail" << std::endl;
			exit(-1);
		}
	}

    void StopSound(FMODAudioComponent& component)
    {
		if (!component.isLoaded) {
			return;
		}

		FMOD::Sound* sound = mSoundMap[component.soundname];
		auto it = mChannelMap.find(sound);
		if (it != mChannelMap.end()) {
			FMOD::Channel* channel = it->second;
            bool isPlaying = false;
            channel->isPlaying(&isPlaying);
            if(isPlaying) {
                channel->stop();
            }

			mChannelMap.erase(it);
		}
    }

	void PauseSound(FMODAudioComponent& component)
	{
		if (!component.isLoaded) {
			return;
		}
		FMOD::Sound* sound = mSoundMap[component.soundname];
		auto it = mChannelMap.find(sound);
		if (it != mChannelMap.end()) {
			FMOD::Channel* channel = it->second;
			bool isPaused = false;
			channel->getPaused(&isPaused);
			if (!isPaused) {
				channel->setPaused(true);
			}
		}
	}

	FMOD::System* mSystem;
    std::unordered_map<std::string, FMOD::Sound*> mSoundMap;
    std::unordered_map<FMOD::Sound*, FMOD::Channel*> mChannelMap;
};