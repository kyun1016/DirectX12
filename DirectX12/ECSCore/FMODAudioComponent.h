#pragma once
#include <string>
#include "../FMODCore/fmod.hpp"

using AudioHandle = uint32_t;

struct FMODAudioComponent {
	static const char* GetName() { return "FMODAudioComponent"; }
	AudioHandle handle = 0;
	float volume = 1.0f;
	bool loop = false;
};

inline void to_json(json& j, const FMODAudioComponent& p) {
	j = json{
		{"handle", p.handle},
		{"volume", p.volume},
		{"loop", p.loop}
	};
}
inline void from_json(const json& j, FMODAudioComponent& p) {
	j.at("handle").get_to(p.handle);
	j.at("volume").get_to(p.volume);
	j.at("loop").get_to(p.loop);
}