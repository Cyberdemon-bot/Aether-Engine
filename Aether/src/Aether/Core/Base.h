#pragma once

#include <memory>
#include <chrono>

#ifdef _WIN32
	#ifdef _WIN64
		#define AETHER_PLATFORM_WINDOWS
	#else
		#error "x86 Builds are not supported!"
	#endif
#elif defined(__APPLE__) || defined(__MACH__)
	#include <TargetConditionals.h>
	#if TARGET_OS_MAC == 1
		#define AETHER_PLATFORM_MACOS
	#else
		#error "MacOS only!"
	#endif
#elif defined(__linux__)
	#define AETHER_PLATFORM_LINUX
#else
	#error "Unknown platform!"
#endif

#ifdef AETHER_SHARED
	#ifdef AETHER_PLATFORM_WINDOWS
		#ifdef AETHER_BUILD_DLL
			#define AETHER_API __declspec(dllexport)
		#else
			#define AETHER_API __declspec(dllimport)
		#endif
	#else
		#ifdef AETHER_BUILD_DLL
			#define AETHER_API __attribute__((visibility("default")))
		#else
			#define AETHER_API __attribute__((visibility("default")))
		#endif
	#endif
#else 
	#define AETHER_API
#endif

#ifdef AETHER_PLATFORM_WINDOWS
	#define AE_DEBUGBREAK() __debugbreak()
#elif defined(AETHER_PLATFORM_LINUX) || defined(AETHER_PLATFORM_MACOS)
	#include <signal.h>
	#define AE_DEBUGBREAK() raise(SIGTRAP)
#else
	#define AE_DEBUGBREAK()
#endif

#ifdef AETHER_DEBUG
	#ifdef AE_ENABLE_ASSERT
		#define AE_ENABLE_ASSERTS
	#endif
#endif

#define AE_EXPAND_MACRO(x) x
#define AE_STRINGIFY_MACRO(x) #x
#define BIT(x) (1 << x)
#define AE_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
#define AE_BIND_CONSOLE_FN(fn) [this](const std::vector<std::string>& args) -> void { return this->fn(args); }
#define AE_REFLECT_THIS_RO(MEMBER) std::make_tuple(#MEMBER, &Self::MEMBER, true)
#define AE_REFLECT_THIS(MEMBER) std::make_tuple(#MEMBER, &Self::MEMBER, false)
#define AE_REFLECT_THIS_OP(OP, A, B) std::make_tuple(#OP, &Self::OP##_##A##_##B)
#define AE_REFLECT_THIS_OP_COM(OP, A, B) AE_REFLECT_THIS_OP(OP, A, B), AE_REFLECT_THIS_OP(OP, B, A)

#define AE_REFLECT_NAME(NAME) \
static constexpr const char* get_name() { \
	return NAME; \
}

#define AE_REFLECT_LIST(...) \
static constexpr auto reflect() { \
    return std::make_tuple(__VA_ARGS__); \
}

#define AE_SCRIPT_OP_LIST(...) \
static constexpr auto script_op() { \
    return std::make_tuple(__VA_ARGS__); \
}

#define AE_SCRIPT_OP(FUNC_NAME, A_NAME, B_NAME, A, B, RES, NATIVE_A, NATIVE_B, OP) \
inline RES FUNC_NAME##_##A_NAME##_##B_NAME(const A& a, const B& b)  { \
	return RES((NATIVE_A)a OP (NATIVE_B)b); \
}

#define AE_SCRIPT_OP_COM(FUNC_NAME, A_NAME, B_NAME, A, B, RES, NATIVE_A, NATIVE_B, OP) \
AE_SCRIPT_OP(FUNC_NAME, A_NAME, B_NAME, A, B, RES, NATIVE_A, NATIVE_B, OP) \
AE_SCRIPT_OP(FUNC_NAME, B_NAME, A_NAME, B, A, RES, NATIVE_B, NATIVE_A, OP) 

#define AE_MAKE_LAMBDA(INP, EXC) [](INP) {EXC}

namespace Aether {

	template<typename T>
	using Scope = std::unique_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Scope<T> CreateScope(Args&& ... args)
	{
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template<typename T>
	using Ref = std::shared_ptr<T>;
	template<typename T, typename ... Args>
	constexpr Ref<T> CreateRef(Args&& ... args)
	{
		return std::make_shared<T>(std::forward<Args>(args)...);
	}

}

#include "Aether/Core/Log.h"

struct ProfileTimer 
{
	const char* name;
	float minTime;
	std::chrono::steady_clock::time_point start;

	ProfileTimer(const char* n, float mt) : name(n), maxTime(mt), start(std::chrono::steady_clock::now()) {}
	~ProfileTimer()
	{
		auto end = std::chrono::steady_clock::now();
		float duration = std::chrono::duration<float, std::milli>(end - start).count();
		if (duration > minTime) AE_CORE_TRACE("{0}: {1}ms", name, duration);
	}
}

#define AE_PROFILE_SCOPE(name) ProfileTimer timer##__LINE__(name)