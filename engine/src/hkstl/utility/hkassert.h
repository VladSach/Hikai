#ifndef HK_ASSERT_H
#define HK_ASSERT_H

#include "hkstl/Logger.h"

#include "platform/predef.h"

using hk_assert_callback = void (*)(const char *, const char *, const char *);
extern hk_assert_callback hk_assert_handler;
HKAPI void hk_set_assert_handler(hk_assert_callback);

// FIX: no idea where to put break define
#ifdef HKMSVC
    #define HKBREAK __debugbreak()
#elif defined(HKCLANG) || defined(HKGNUC)
    #if __has_builtin(__builtin_debugtrap)
        #define HKBREAK __builtin_debugtrap()
    #else
        #include <signal.h>
        #if defined(SIGTRAP)
            #define HKBREAK raise(SIGTRAP)
        #else
            #define HKBREAK raise(SIGABRT)
        #endif
    #endif
#else
    #define HKBREAK asm("int $3")
#endif

#define STATIC_ASSERT static_assert

#define PANIC(title, reason, ...)                     \
{                                                     \
    ((*hk_assert_handler)(                            \
        hk::log::to_string_va(title).c_str(),         \
        hk::log::to_string_va(reason).c_str(),        \
        hk::log::to_string_va(__VA_ARGS__).c_str())); \
    hk::log::dispatch();                              \
    HKBREAK;                                          \
}

#if defined(_MSVC_TRADITIONAL)
    #define LOG_FATAL_HELPER(message, ...) \
        LOG(hk::log::Level::LVL_FATAL, message, __VA_ARGS__)

    #define ALWAYS_ASSERT(expression, ...)                             \
    {                                                                  \
        if (!(expression)) {                                           \
            LOG_FATAL_HELPER("Assertion failed:", __VA_ARGS__);        \
            PANIC("Hikai Assertion Fail", #expression, ##__VA_ARGS__); \
        }                                                              \
    }
#else
    #define ALWAYS_ASSERT(expression, ...)                             \
    {                                                                  \
        if (!(expression)) {                                           \
            LOG_FATAL("Assertion failed:", ##__VA_ARGS__);             \
            PANIC("Hikai Assertion Fail", #expression, ##__VA_ARGS__); \
        }                                                              \
    }
#endif

#ifndef HKDEBUG
#define DEV_PANIC(...)
#define DEV_ASSERT(...)
#else
#define DEV_PANIC(reason, ...) PANIC("DEV Assert", reason, ##__VA_ARGS__)
#define DEV_ASSERT(expression, ...) ALWAYS_ASSERT(expression, ##__VA_ARGS__)
#endif

#endif // HK_ASSERT_H
