#ifndef HK_LOGGER_H
#define HK_LOGGER_H

#include <string>
#include <cstring>
#include <sstream>
#include <functional>

#include "utility/hktypes.h"

#include "hkcommon.h"

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#ifdef _MSC_VER
    #define FUNCTION_SIGNATURE __FUNCSIG__
#elif defined(__GNUC__)
    #define FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#else
    #define FUNCTION_SIGNATURE __func__
#endif

#define LOG(level, ...)                      \
    hk::log::log(                            \
    {                                        \
        level,                               \
        FUNCTION_SIGNATURE,                  \
        std::strrchr("/" __FILE__, '/') + 1, \
        STRINGIZE(__LINE__),                 \
        hk::log::to_string_va(__VA_ARGS__)   \
    })

#define LOG_FATAL(...) LOG(hk::log::Level::LVL_FATAL, __VA_ARGS__)
#define LOG_ERROR(...) LOG(hk::log::Level::LVL_ERROR, __VA_ARGS__)
#define LOG_WARN(...) LOG(hk::log::Level::LVL_WARN, __VA_ARGS__)
#define LOG_INFO(...) LOG(hk::log::Level::LVL_INFO, __VA_ARGS__)

#ifndef HKDEBUG
    #define LOG_DEBUG(...)
    #define LOG_TRACE(...)
#else
    #define LOG_DEBUG(...) LOG(hk::log::Level::LVL_DEBUG, __VA_ARGS__)
    #define LOG_TRACE(...) LOG(hk::log::Level::LVL_TRACE, __VA_ARGS__)
#endif

namespace hk::log {

enum class Level {
    LVL_FATAL,
    LVL_ERROR,
    LVL_WARN,
    LVL_INFO,
    LVL_DEBUG,
    LVL_TRACE,

    MAX_LVL
};

// Info left by log function
struct Log {
    Level level;
    std::string caller;
    std::string file;
    std::string line;
    std::string time;
    std::string args;
};

// Modified message sent to handlers
struct MsgInfo {
    Level level;
    const std::string &caller;
    const std::string &file_path;
    const std::string &line_number;
    const std::string &args;
};

HKAPI void init();
HKAPI void deinit();

HKAPI void log(const MsgInfo &info);

using LoggerCallback = std::function<void(const hk::log::Log &log)>;
HKAPI u32 add_message_handler(LoggerCallback callback);
HKAPI void remove_message_handler(u32 handle);

// ===== HIKAI INTERNAL USE =====
HKAPI void dispatch();

template <typename... Args>
inline std::string to_string_va(const Args& ...args)
{
    std::ostringstream oss;
    ((oss << args << " "), ...);

    // Remove trailing space
    std::string res = oss.str();
    if (!res.empty()) { res.pop_back(); }

    return res;
}

// #ifdef HKDEBUG
struct DebugInfo {
    u32 logs_issued = 0;
};

HKAPI const DebugInfo& get_debug_info();
// #endif

}

#endif // HK_LOGGER_H
