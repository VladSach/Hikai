#include "utils/Logger.h"

#include <chrono>
#include <iomanip>

Logger *Logger::singleton = nullptr;

Logger *Logger::getInstance()
{
    if (singleton == nullptr) {
        singleton = new Logger();
        singleton->init();
    }
    return singleton;
}

void Logger::init()
{
    LOG_INFO("Logger initialized");
}

void Logger::deinit()
{
    delete singleton;
}

void Logger::addMessageHandler(LoggerHandlerCallback callback)
{
    if (cntHandlers >= maxHandlers) {
        LOG_WARN("Currently supported only up to", maxHandlers, "handlers");
        return;
    }
    handlers[cntHandlers++] = callback;
}

void Logger::removeMessageHandler(LoggerHandlerCallback callback)
{
    unsigned i;
    for (i = 0; i < cntHandlers; ++i) {
        if (handlers[i] == callback) { break; }
    }

    --cntHandlers;
    memset(&handlers[i], 0, sizeof(handlers[i]));
}

void Logger::log(const MsgInfo &info)
{
    if (!cntHandlers) return;

    bool is_error = info.level < Level::LVL_WARN;
    bool is_trace = info.level == Level::LVL_TRACE;
    int log_lvl = static_cast<int>(info.level);

    std::time_t now = std::time(nullptr);
    std::tm tm;
    char time_str[32];
    localtime_s(&tm, &now);
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm);

    std::string caller = info.callerName;
#ifdef _MSC_VER
    // Removes all __cdecl instances from __FUNCSIG__
    std::string r = "__cdecl ";

    for (std::string::size_type i = caller.find(r);
         i != std::string::npos;
         i = caller.find(r))
    {
        caller.erase(i, r.length());
    }
#endif

    if (caller.size() > MaxFuncNameLength) {
        caller.erase(MaxFuncNameLength - 3, std::string::npos);
        caller.append("...");
    }

    // Obtaining only file name instead of full path
    std::string file = info.fileName.substr(
                            info.fileName.find_last_of("/\\") + 1) + ":";

    MsgAddInfo misc {is_error, is_trace, log_lvl, time_str, caller, file};

    for (unsigned i = 0; i < maxHandlers; ++i) {
        if (!handlers[i]) { continue; }

        handlers[i](info, misc);
    }
}
