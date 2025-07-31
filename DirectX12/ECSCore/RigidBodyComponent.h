#pragma once
#include "ECSConfig.h"

struct RigidBodyComponent {
	static const char* GetName() { return "RigidBodyComponent"; }
	float Mass;
	float3 DiffPosition;
	float3 Velocity;
	float3 AngularVelocity;
	float3 Force;
	float3 Torque;
	float3 Acceleration;
	float3 AngularAcceleration;
	bool UseGravity = true;
	bool IsKinematic = false;
};

inline void to_json(json& j, const RigidBodyComponent& p) {
	j = json{
		{"Mass", p.Mass},
		{"DiffPosition", {p.DiffPosition.x, p.DiffPosition.y, p.DiffPosition.z}},
		{"Velocity", {p.Velocity.x, p.Velocity.y, p.Velocity.z}},
		{"AngularVelocity", {p.AngularVelocity.x, p.AngularVelocity.y, p.AngularVelocity.z}},
		{"Force", {p.Force.x, p.Force.y, p.Force.z}},
		{"Torque", {p.Torque.x, p.Torque.y, p.Torque.z}},
		{"Acceleration", {p.Acceleration.x, p.Acceleration.y, p.Acceleration.z}},
		{"AngularAcceleration", {p.AngularAcceleration.x, p.AngularAcceleration.y, p.AngularAcceleration.z}},
		{"UseGravity", p.UseGravity},
		{"IsKinematic", p.IsKinematic}
	};
}
inline void from_json(const json& j, RigidBodyComponent& p) {
	j.at("Mass").get_to(p.Mass);
	j.at("DiffPosition").get_to(p.DiffPosition.x);
	j.at("Velocity").get_to(p.Velocity.x);
	j.at("AngularVelocity").get_to(p.AngularVelocity.x);
	j.at("Force").get_to(p.Force.x);
	j.at("Torque").get_to(p.Torque.x);
	j.at("Acceleration").get_to(p.Acceleration.x);
	j.at("AngularAcceleration").get_to(p.AngularAcceleration.x);
	p.UseGravity = j.at("UseGravity").get<bool>();
	p.IsKinematic = j.at("IsKinematic").get<bool>();
}