#ifndef HK_LOGGER_H
#define HK_LOGGER_H

#include <string>
#include <sstream>

#define STRINGIZE_DETAIL(x) #x
#define STRINGIZE(x) STRINGIZE_DETAIL(x)

#ifdef _MSC_VER
    #define FUNCTION_SIGNATURE __FUNCSIG__
#elif defined(__GNUC__)
    #define FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#else
    #define FUNCTION_SIGNATURE __func__
#endif

#define LOG(level, ...) \
    Logger::getInstance()->log({level, FUNCTION_SIGNATURE, \
                               std::strrchr("/" __FILE__, '/') + 1, \
                               STRINGIZE(__LINE__), \
                               Logger::getInstance()->getArgs(__VA_ARGS__)}) \

#define LOG_FATAL(...) LOG(Logger::Level::LVL_FATAL, __VA_ARGS__)
#define LOG_ERROR(...) LOG(Logger::Level::LVL_ERROR, __VA_ARGS__)
#define LOG_WARN(...) LOG(Logger::Level::LVL_WARN, __VA_ARGS__)
#define LOG_INFO(...) LOG(Logger::Level::LVL_INFO, __VA_ARGS__)
#ifndef HKDEBUG
#define LOG_DEBUG(...)
#define LOG_TRACE(...)
#else
#define LOG_DEBUG(...) LOG(Logger::Level::LVL_DEBUG, __VA_ARGS__)
#define LOG_TRACE(...) LOG(Logger::Level::LVL_TRACE, __VA_ARGS__)
#endif

// NOTE: Since logger is its own thing can't include here anything
// maybe should come up with workaround for that
#ifdef HKDLL_OUT
    #ifdef _MSC_VER
    #define HKAPI __declspec(dllexport)
    #else
    #define HKAPI
    #endif
#else
    #ifdef _MSC_VER
    #define HKAPI __declspec(dllimport)
    #else
    #define HKAPI
    #endif
#endif

class HKAPI Logger {
protected:
    Logger() {}
    static Logger *singleton;

public:
    enum class Level {
        LVL_FATAL,
        LVL_ERROR,
        LVL_WARN,
        LVL_INFO,
        LVL_DEBUG,
        LVL_TRACE,

        max_levels
    };

    static constexpr char const *
        lookup_level[static_cast<int> (Level::max_levels)] =
    {
        "[FATAL]:",
        "[ERROR]:",
        "[WARN]:",
        "[INFO]:",
        "[DEBUG]:",
        "[TRACE]:"
    };

    struct MsgInfo {
        Level level;
        const std::string &callerName;
        const std::string &fileName;
        const std::string &lineNumber;
        const std::string &args;
    };

    struct MsgAddInfo {
        bool is_error;
        bool is_trace;
        int log_lvl;
        std::string time;
        std::string caller;
        std::string file;
    };

    using LoggerHandlerCallback = void (*)(const Logger::MsgInfo& info,
                                           const Logger::MsgAddInfo &misc);

    static constexpr int MaxFuncNameLength = 45;

private:
    unsigned cntHandlers = 0;
    static constexpr unsigned maxHandlers = 5;
    LoggerHandlerCallback handlers[maxHandlers];

public:
    Logger(Logger &other) = delete;
    void operator=(const Logger&) = delete;
    static Logger *getInstance();

    void init();
    void deinit();

    void log(const MsgInfo &info);

    void addMessageHandler(LoggerHandlerCallback callback);
    void removeMessageHandler(LoggerHandlerCallback callback);

    template <typename... Args>
    inline std::string getArgs(const Args& ...args) const
    {
        std::ostringstream oss;
        ((oss << args << " "), ...);
        return oss.str();
    }

};
#endif // HK_LOGGER_H
