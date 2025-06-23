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
#include "LogCore.h"

namespace ECS
{
	// namespace Entity
	using Entity = std::uint32_t;
	static constexpr Entity MAX_ENTITIES = 5000;
	using ComponentType = std::uint8_t;
	static constexpr ComponentType MAX_COMPONENTS = 32;
	using Signature = std::bitset<MAX_COMPONENTS>;
	using RepoHandle = std::uint32_t;
}

#define ENUM_OPERATORS_32(T)                 \
inline bool operator&(T a, T b)              \
{                                            \
    return ((uint32_t)a & (uint32_t)b) != 0; \
}                                            \
                                             \
inline T& operator&=(T& a, T b)              \
{                                            \
    a = (T)((uint32_t)a & (uint32_t)b);      \
    return a;                                \
}                                            \
                                             \
inline T operator|(T a, T b)                 \
{                                            \
    return (T)((uint32_t)a | (uint32_t)b);   \
}                                            \
                                             \
inline T& operator |= (T& lhs, T rhs)        \
{                                            \
    lhs = (T)((uint32_t)lhs | (uint32_t)rhs);\
    return lhs;                              \
}                                            \
                                             \
inline T operator~(T a)                      \
{                                            \
    return (T)~((uint32_t)a);                \
}


#define DEFAULT_SINGLETON(SystemClassName)                        \
public:                                                           \
    inline static SystemClassName& GetInstance() {                \
        static SystemClassName instance;                          \
        return instance;                                          \
    }                                                             \
private:                                                          \
    SystemClassName() = default;                                  \
    ~SystemClassName() = default;                                 \
    SystemClassName(const SystemClassName&) = delete;             \
    SystemClassName& operator=(const SystemClassName&) = delete;  \
    SystemClassName(SystemClassName&&) = delete;                  \
    SystemClassName& operator=(SystemClassName&&) = delete;       