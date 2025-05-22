#pragma once
#include "../EngineCore/SimpleMath.h"
#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <array>
#include <set>
#include <bitset>
#include <cassert>
#include <typeindex>
#include <typeinfo>

namespace ECS
{
	// namespace Entity
	using Entity = std::uint32_t;
	static constexpr Entity MAX_ENTITIES = 5000;
	using ComponentType = std::uint8_t;
	static constexpr ComponentType MAX_COMPONENTS = 32;
	using Signature = std::bitset<MAX_COMPONENTS>;
}