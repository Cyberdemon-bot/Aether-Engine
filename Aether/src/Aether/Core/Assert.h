#pragma once

#include "Aether/Core/Base.h"
#include "Aether/Core/Log.h"
#include <filesystem>

#ifdef AE_ENABLE_ASSERTS

	#define AE_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { AE##type##ERROR(msg, __VA_ARGS__); AE_DEBUGBREAK(); } }
	#define AE_INTERNAL_ASSERT_WITH_MSG(type, check, ...) AE_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
	#define AE_INTERNAL_ASSERT_NO_MSG(type, check) AE_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", AE_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define AE_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define AE_INTERNAL_ASSERT_GET_MACRO(...) AE_EXPAND_MACRO( AE_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, AE_INTERNAL_ASSERT_WITH_MSG, AE_INTERNAL_ASSERT_NO_MSG) )

	#define AE_ASSERT(...) AE_EXPAND_MACRO( AE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__) )
	#define AE_CORE_ASSERT(...) AE_EXPAND_MACRO( AE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, __VA_ARGS__) )

#else
	#define AE_ASSERT(...)
	#define AE_CORE_ASSERT(...)
#endif