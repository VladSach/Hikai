#ifndef HK_ASSERT_H
#define HK_ASSERT_H

#include "hkstl/Logger.h"
#include "platform/utils.h"

#define STATIC_ASSERT static_assert

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
        }
#else
    // TODO: choose between message box or task dialog
    #define PANIC(title, reason, ...)            \
    {                                            \
        LOG_FATAL(title, ##__VA_ARGS__);         \
        hk::platform::addTaskDialog(             \
            hk::log::argsToString(title),        \
            hk::log::argsToString(reason),       \
            hk::log::argsToString(__VA_ARGS__)); \
        HKBREAK;                                 \
    }

    #define ALWAYS_ASSERT(expression, ...)                             \
        if (!(expression)) {                                           \
            LOG_FATAL("Assertion failed:", ##__VA_ARGS__);             \
            PANIC("Hikai Assertion Fail", #expression, ##__VA_ARGS__); \
        }

#endif

#ifndef HKDEBUG
#define DEV_PANIC(...)
#define DEV_ASSERT(...)
#else
#define DEV_PANIC(reason, ...) PANIC(reason, __VA_ARGS__)
#define DEV_ASSERT(expression, ...) ALWAYS_ASSERT(expression, __VA_ARGS__)
#endif

#endif // HK_ASSERT_H
