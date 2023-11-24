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

#define LOG(level, message, ...) \
    Logger::getInstance()->log({level, FUNCTION_SIGNATURE, \
                               std::strrchr("/" __FILE__, '/') + 1, STRINGIZE(__LINE__), message, \
                               Logger::getInstance()->getArgs(__VA_ARGS__)}) \

#define LOG_FATAL(message, ...) LOG(Logger::Level::LVL_FATAL, message, __VA_ARGS__)
#define LOG_ERROR(message, ...) LOG(Logger::Level::LVL_ERROR, message, __VA_ARGS__)
#define LOG_WARN(message, ...) LOG(Logger::Level::LVL_WARN, message, __VA_ARGS__)
#define LOG_INFO(message, ...) LOG(Logger::Level::LVL_INFO, message, __VA_ARGS__)
#ifndef HKDEBUG
#define LOG_DEBUG(message, ...)
#define LOG_TRACE(message, ...)
#else
#define LOG_DEBUG(message, ...) LOG(Logger::Level::LVL_DEBUG, message, __VA_ARGS__)
#define LOG_TRACE(message, ...) LOG(Logger::Level::LVL_TRACE, message, __VA_ARGS__)
#endif

// Since logger its own thing can't include here anything
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

	static constexpr char const *lookup_level[static_cast<int>(Level::max_levels)] = {
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
		const std::string &message;
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

    using LoggerHandlerCallback = void (*)(void *self,
                                           const Logger::MsgInfo& info,
                                           const Logger::MsgAddInfo &misc);

    struct Handler {
        void *self = nullptr;
        LoggerHandlerCallback callback = nullptr;
    };

private:
    unsigned cntHandlers = 0; 
    Handler handlers[5];

public:
	Logger(Logger &other) = delete;
	void operator=(const Logger&) = delete;
	static Logger *getInstance();

    void init();
	void deinit();

    void log(const MsgInfo &info);

    void addMessageHandler(void *self, LoggerHandlerCallback callback);
    void removeMessageHandler(void *self, LoggerHandlerCallback callback);

    template <typename... Args>
    inline std::string getArgs(const Args& ...args) const
    {
        std::ostringstream oss;
        ((oss << args << " "), ...);
        return oss.str();
    }

};
#endif // HK_LOGGER_H
