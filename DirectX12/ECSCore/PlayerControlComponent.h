#pragma once

struct PlayerControlComponent {
	static const char* GetName() { return "PlayerControlComponent"; }
    float moveSpeed = 10.0f;
};

inline void to_json(json& j, const PlayerControlComponent& p) {
	j = json{
		{"moveSpeed", p.moveSpeed}
	};
}
inline void from_json(const json& j, PlayerControlComponent& p) {
	j.at("moveSpeed").get_to(p.moveSpeed);
}