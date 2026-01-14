#pragma once

#include <memory>

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

#ifdef AETHER_PLATFORM_WINDOWS
	#ifdef AETHER_BUILD_DLL
		#define AETHER_API __declspec(dllexport)
	#else
		#define AETHER_API __declspec(dllimport)
	#endif
#else
	#define AETHER_API __attribute__((visibility("default")))
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
	#define AE_ENABLE_ASSERTS
#endif

#define AE_EXPAND_MACRO(x) x
#define AE_STRINGIFY_MACRO(x) #x
#define BIT(x) (1 << x)
#define AE_BIND_EVENT_FN(fn) [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }


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
#include "Aether/Core/Assert.h"