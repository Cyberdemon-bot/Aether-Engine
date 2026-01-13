#pragma once

#include "Base.h"
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"


namespace Aether {

	class AETHER_API Log
	{
	public:
		static void Init();

		static Ref<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		static Ref<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static Ref<spdlog::logger> s_CoreLogger;
		static Ref<spdlog::logger> s_ClientLogger;
	};
	template<typename T, typename = void>
	struct has_to_string : std::false_type {};

	template<typename T>
	struct has_to_string<T, std::void_t<decltype(std::declval<T>().ToString())>> : std::true_type {};

	template<typename T>
	decltype(auto) ConvertLogArg(const T& arg)
	{
		if constexpr (has_to_string<T>::value) {
			return arg.ToString();
		}
		else {
			return arg;
		}
	}

	template<typename... Args>
	void LogWrapper(std::shared_ptr<spdlog::logger>& logger, spdlog::level::level_enum level, Args&&... args)
	{
		logger->log(level, ConvertLogArg(std::forward<Args>(args))...);
	}
}

#define AE_CORE_TRACE(...)   ::Aether::LogWrapper(::Aether::Log::GetCoreLogger(), spdlog::level::trace, __VA_ARGS__)
#define AE_CORE_INFO(...)    ::Aether::LogWrapper(::Aether::Log::GetCoreLogger(), spdlog::level::info, __VA_ARGS__)
#define AE_CORE_WARN(...)    ::Aether::LogWrapper(::Aether::Log::GetCoreLogger(), spdlog::level::warn, __VA_ARGS__)
#define AE_CORE_ERROR(...)   ::Aether::LogWrapper(::Aether::Log::GetCoreLogger(), spdlog::level::err, __VA_ARGS__)
#define AE_CORE_FATAL(...)   ::Aether::LogWrapper(::Aether::Log::GetCoreLogger(), spdlog::level::critical, __VA_ARGS__)

// Client log macros
#define AE_TRACE(...)        ::Aether::LogWrapper(::Aether::Log::GetClientLogger(), spdlog::level::trace, __VA_ARGS__)
#define AE_INFO(...)         ::Aether::LogWrapper(::Aether::Log::GetClientLogger(), spdlog::level::info, __VA_ARGS__)
#define AE_WARN(...)         ::Aether::LogWrapper(::Aether::Log::GetClientLogger(), spdlog::level::warn, __VA_ARGS__)
#define AE_ERROR(...)        ::Aether::LogWrapper(::Aether::Log::GetClientLogger(), spdlog::level::err, __VA_ARGS__)
#define AE_FATAL(...)        ::Aether::LogWrapper(::Aether::Log::GetClientLogger(), spdlog::level::critical, __VA_ARGS__)