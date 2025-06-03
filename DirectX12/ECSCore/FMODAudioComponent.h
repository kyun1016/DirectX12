#pragma once
#include <string>
#include "../FMODCore/fmod.hpp"

using AudioHandle = uint32_t;

struct FMODAudioComponent {
	AudioHandle handle;
	float volume = 1.0f;
	bool loop = false;
};

//struct FMODAudioComponent { 
//	std::string filename;
//	std::string soundname;
//	float volume = 1.0f;
//	bool playStart = false;
//	bool useLoop = false;
//	bool isLoaded = false;
//};