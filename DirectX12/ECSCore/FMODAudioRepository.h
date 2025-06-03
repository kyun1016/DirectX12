#pragma once

#include "FMODAudioComponent.h"
#include <unordered_map>
#include <iostream>

class FMODAudioRepository {
public:
	static void Init(FMOD::System* system) {
		mSystem = system;
	}

	static AudioHandle LoadSound(const std::string& name, const std::string& path, bool loop) {
		if (sNameToHandle.find(name) != sNameToHandle.end()) {
			sSoundEntries[sNameToHandle[name]].refCount++;
			return sNameToHandle[name];
		}

		int loopFlag = loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF;
		loopFlag |= FMOD_DEFAULT;

		FMOD::Sound* sound = nullptr;
		FMOD_RESULT result = mSystem->createSound(path.c_str(), loopFlag, nullptr, &sound);
		if (result != FMOD_OK) {
			std::cerr << "FMOD: Failed to load sound: " << path << "\n";
			return 0;
		}

		AudioHandle handle = sNextHandle++;
		sNameToHandle[name] = handle;
		sSoundEntries[handle] = { sound, 1 };
		return handle;
	}

	static void Play(AudioHandle handle, float volume = 1.0f) {
		auto it = sSoundEntries.find(handle);
		if (it == sSoundEntries.end()) return;

		FMOD::Channel* ch = nullptr;
		if (mSystem->playSound(it->second.sound, nullptr, false, &ch) == FMOD_OK && ch)
			ch->setVolume(volume);
	}

	static void Shutdown() {
		for (auto& [_, entry] : sSoundEntries) {
			entry.sound->release();
		}
		sSoundEntries.clear();
		sNameToHandle.clear();
		sNextHandle = 1;
	}

private:
	struct SoundEntry {
		FMOD::Sound* sound;
		int refCount;
	};

	static inline FMOD::System* mSystem = nullptr;
	static inline std::unordered_map<std::string, AudioHandle> sNameToHandle;
	static inline std::unordered_map<AudioHandle, SoundEntry> sSoundEntries;
	static inline AudioHandle sNextHandle = 1;
};