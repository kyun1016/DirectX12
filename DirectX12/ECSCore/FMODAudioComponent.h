#pragma once
#include <string>
#include "../FMODCore/fmod.hpp"

struct FMODAudioComponent { 
	std::string filename;
	std::string soundname;
	float volume = 1.0f;
	bool playStart = false;
	bool useLoop = false;
	bool isLoaded = false;
};