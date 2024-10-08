#include "utils/Logger.h"

#include "defines.h"

#include "utils/containers/hkvector.h"
#include "utils/containers/hkring_buffer.h"

#include <chrono>
#include <iomanip>
#include <forward_list>

namespace hk::log {

// For now handles will just be indexes
// Don't even think there is a need for more complicated logic
u32 cntHandlers = 0;
/* PERF: Could be 2 vectors as well, inspect which option is faster
 * https://stackoverflow.com/questions/77546017/
   best-c-data-structure-for-fast-iteration-and-fast-insertion-removal-time
*/
static std::forward_list<LoggerCallback> handlers;

// Messages set to initial size and then all unprinted messages got deleted?
static hk::ring_buffer<Log, 100, true> logs;
// Or should better use dynamic array for having an ability to dump logs
// all at one to some file instead of constantly adding to it
// Guess in that case handler should save all received the logs in himself <--- THIS IDEA

void init()
{
    LOG_INFO("Logger initialized");
    handlers.clear();
    cntHandlers = 0;
}

void deinit()
{
}

u32 addMessageHandler(LoggerCallback callback)
{
    if (handlers.empty()) {
        handlers.push_front(callback);
        return cntHandlers++;
    }

    auto it = handlers.begin();
    std::advance(it, cntHandlers - 1);
    handlers.insert_after(it, callback);
    return cntHandlers++;
}

void removeMessageHandler(u32 handle)
{
    if (handle == 0) {
        handlers.pop_front();
        --cntHandlers;
        return;
    }
    auto it = handlers.begin();
    std::advance(it, handle - 1);
    handlers.erase_after(it);
    --cntHandlers;
}

void log(const MsgInfo &info)
{
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

    // Obtaining only file name instead of full path
    std::string file = info.filePath.substr(info.filePath.find_last_of("/\\") + 1);

    Log log = { info.level, caller, file, info.lineNumber, time_str, info.args };

    logs.push(log);
}

void dispatch()
{
    Log log;
    while (logs.pop(log)) {
        for (auto &handle : handlers) {
            handle(log);
        }
    }
}

inline std::string levelToString(const Level &level)
{
    constexpr char const *lookup_level[] =
    {
        "[FATAL]:",
        "[ERROR]:",
        "[WARN]:",
        "[INFO]:",
        "[DEBUG]:",
        "[TRACE]:"
    };

    return lookup_level[static_cast<u32>(level)];
}

}
