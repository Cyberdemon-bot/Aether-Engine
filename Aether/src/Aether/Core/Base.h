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

	template <typename Tuple, typename Func>
    constexpr void ForEachTuple(Tuple&& t, Func&& f) {
        std::apply([&f](auto&&... args) {
            (f(std::forward<decltype(args)>(args)), ...);
        }, std::forward<Tuple>(t));
    }
}

#define AE_UNWRAP(...) __VA_ARGS__
#define AE_MAKE_LAMBDA(ENV, INP, RET, ...) [AE_UNWRAP ENV](AE_UNWRAP INP) -> RET { __VA_ARGS__ }

#define AE_REFLECT_LIST(...) std::make_tuple(__VA_ARGS__)
#define AE_REFLECT(NAME, ...) std::make_tuple(NAME, std::make_tuple(__VA_ARGS__))

template <typename T>
constexpr std::string_view GetTypeName() {
#if defined(__clang__) || defined(__GNUC__)
    std::string_view name = __PRETTY_FUNCTION__;
    size_t start = name.find("T = ") + 4;
    size_t end = name.find_last_of(']');
    return name.substr(start, end - start);
#elif defined(_MSC_VER)
    std::string_view name = __FUNCSIG__;
    size_t start = name.find("<") + 1;
    size_t end = name.find_last_of('>');
    return name.substr(start, end - start);
#endif
}