#pragma once

struct TimeComponent {
    static const char* GetName() { return "TimeComponent"; }
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
    float fixedDeltaTime = 0.016f; // 60fps 기준
};

struct AnimationTimeComponent {
    static const char* GetName() { return "AnimationTimeComponent"; }
    float localTime = 0.0f;
    float speed = 1.0f;
};

inline void to_json(json& j, const TimeComponent& p)
{
	j = json{
		{"deltaTime", p.deltaTime},
		{"totalTime", p.totalTime},
		{"fixedDeltaTime", p.fixedDeltaTime}
	};
}
inline void from_json(const json& j, TimeComponent& p)
{
	p.deltaTime = j.at("deltaTime").get<float>();
	p.totalTime = j.at("totalTime").get<float>();
	p.fixedDeltaTime = j.at("fixedDeltaTime").get<float>();
}

inline void to_json(json& j, const AnimationTimeComponent& p)
{
	j = json{
		{"localTime", p.localTime},
		{"speed", p.speed}
	};
}
inline void from_json(const json& j, AnimationTimeComponent& p)
{
	p.localTime = j.at("localTime").get<float>();
	p.speed = j.at("speed").get<float>();
}