#ifndef HK_DEBUG_H
#define HK_DEBUG_H

#include "utils/Logger.h"
#include "platform/utils.h"

#include <cstdlib>

#define HKBREAK __debugbreak()

#if _MSVC_TRADITIONAL
    #define LOG_FATAL_HELPER(message, ...) \
        LOG(hk::log::Level::LVL_FATAL, message, __VA_ARGS__)

    #define ALWAYS_ASSERT(expression, ...) \
        if (!(expression)) { \
            LOG_FATAL_HELPER("Assertion failed:", __VA_ARGS__); \
            hk::platform::addMessageBox( \
                "Hikai Assertion Fail", \
                hk::log::argsToString(__VA_ARGS__).c_str()); \
            HKBREAK; \
            std::abort(); \
        }
#else
    #define ALWAYS_ASSERT(expression, ...) \
        if (!(expression)) { \
            LOG_FATAL("Assertion failed:", ##__VA_ARGS__); \
            hk::log::dispatch(); \
            hk::platform::addMessageBox( \
                "Hikai Assertion Fail", \
                hk::log::argsToString(__VA_ARGS__).c_str()); \
            HKBREAK; \
            std::abort(); \
        }
#endif

#ifndef HKDEBUG
#define DEV_ASSERT(...)
#else
#define DEV_ASSERT(expression, ...) ALWAYS_ASSERT(expression, __VA_ARGS__);
#endif

#endif // HK_DEBUG_H
