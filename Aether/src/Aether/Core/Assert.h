#pragma once

#include "Aether/Core/Base.h"
#include <filesystem>

namespace Aether {
    class Log;
}

#ifdef AE_ENABLE_ASSERTS

    // Core assert - message is REQUIRED
    #define AE_CORE_ASSERT(check, msg, ...) \
        { if(!(check)) { \
            ::Aether::Log::GetCoreLogger()->error(msg, ##__VA_ARGS__); \
            AE_DEBUGBREAK(); \
        } }
    
    // Client assert - message is REQUIRED
    #define AE_ASSERT(check, msg, ...) \
        { if(!(check)) { \
            ::Aether::Log::GetClientLogger()->error(msg, ##__VA_ARGS__); \
            AE_DEBUGBREAK(); \
        } }

#else
    #define AE_CORE_ASSERT(...)
    #define AE_ASSERT(...)
#endif