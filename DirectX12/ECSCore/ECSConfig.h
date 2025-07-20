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

// DX12
#include "DX12_Config.h"
// DX11
// TODO #include "DX11_Config.h"
// Vulkan
// TODO #include "Vulkan_Config.h"


namespace ECS
{
	// namespace Entity
	using Entity = std::uint32_t;
	static constexpr Entity MAX_ENTITIES = 5000;
	using ComponentType = std::uint8_t;
	static constexpr ComponentType MAX_COMPONENTS = 32;
	using Signature = std::bitset<MAX_COMPONENTS>;
	using RepoHandle = std::uint32_t;
    using Handle = std::size_t;
}

#define ENUM_OPERATORS_64(T)                  \
inline bool operator&(T a, T b)               \
{                                             \
    return ((uint64_t)a & (uint64_t)b) != 0;  \
}                                             \
inline T& operator&=(T& a, T b)               \
{                                             \
    a = (T)((uint64_t)a & (uint64_t)b);       \
    return a;                                 \
}                                             \
inline T operator|(T a, T b)                  \
{                                             \
    return (T)((uint64_t)a | (uint64_t)b);    \
}                                             \
inline T& operator |= (T& lhs, T rhs)         \
{                                             \
    lhs = (T)((uint64_t)lhs | (uint64_t)rhs); \
    return lhs;                               \
}                                             \
inline T operator~(T a)                       \
{                                             \
    return (T)~((uint64_t)a);                 \
}                                             

#define ENUM_OPERATORS_32(T)                 \
inline bool operator&(T a, T b)              \
{                                            \
    return ((uint32_t)a & (uint32_t)b) != 0; \
}                                            \
inline T& operator&=(T& a, T b)              \
{                                            \
    a = (T)((uint32_t)a & (uint32_t)b);      \
    return a;                                \
}                                            \
inline T operator|(T a, T b)                 \
{                                            \
    return (T)((uint32_t)a | (uint32_t)b);   \
}                                            \
inline T& operator |= (T& lhs, T rhs)        \
{                                            \
    lhs = (T)((uint32_t)lhs | (uint32_t)rhs);\
    return lhs;                              \
}                                            \
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

#define DEFAULT_CLASS(SystemClassName)                            \
public:                                                           \
    SystemClassName() = default;                                  \
    ~SystemClassName() = default;                                 \
    SystemClassName(const SystemClassName&) = delete;             \
    SystemClassName& operator=(const SystemClassName&) = delete;  \
    SystemClassName(SystemClassName&&) = delete;                  \
    SystemClassName& operator=(SystemClassName&&) = delete;       


enum class eRenderLayer : std::uint64_t
{
	None = 0,
	Opaque = 1 << 0,
	Sprite = 1 << 1,
	SkinnedOpaque = 1 << 2,
	Mirror = 1 << 3,
	Reflected = 1 << 4,
	AlphaTested = 1 << 5,
	Transparent = 1 << 6,
	Subdivision = 1 << 7,
	Normal = 1 << 8,
	SkinnedNormal = 1 << 9,
	TreeSprites = 1 << 10,
	Tessellation = 1 << 11,
	BoundingBox = 1 << 12,
	BoundingSphere = 1 << 13,
	CubeMap = 1 << 14,
	DebugShadowMap = 1 << 15,
	OpaqueWireframe = 1 << 16,
	MirrorWireframe = 1 << 17,
	ReflectedWireframe = 1 << 18,
	AlphaTestedWireframe = 1 << 19,
	TransparentWireframe = 1 << 20,
	SubdivisionWireframe = 1 << 21,
	NormalWireframe = 1 << 22,
	TreeSpritesWireframe = 1 << 23,
	TessellationWireframe = 1 << 24,
	ShadowMap = 1 << 25,
	SkinnedShadowMap = 1 << 26,
	AddCS = 1 << 27,
	BlurCS = 1 << 28,
	WaveCS = 1 << 29,
	ShaderToy = 1 << 30,
	Test = 1 << 31
};
ENUM_OPERATORS_64(eRenderLayer)