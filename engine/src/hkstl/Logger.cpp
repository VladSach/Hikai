#include "Logger.h"

#include "containers/hkvector.h"
#include "containers/hkring_buffer.h"

#include <chrono>
#include <iomanip>
#include <forward_list>

namespace hk::log {

// For now handles will just be indexes
// Don't even think there is a need for more complex logic
static u32 cnt_handlers = 0;

/* PERF: Could be 2 vectors as well, inspect which option is faster
 * https://stackoverflow.com/questions/77546017/
   best-c-data-structure-for-fast-iteration-and-fast-insertion-removal-time
 */
static std::forward_list<LoggerCallback> handlers;

// Instead of using dynamic array here,
// handlers should store all the logs they need in themselves.
static hk::ring_buffer<Log, 100, true> logs;

// #ifdef HKDEBUG
static DebugInfo debug_info;
// #endif

void init()
{
    LOG_INFO("Logger initialized");
    handlers.clear();
    cnt_handlers = 0;
}

void deinit()
{
    LOG_INFO("Logger deinitialized");
    handlers.clear();
    cnt_handlers = 0;
}

u32 add_message_handler(LoggerCallback callback)
{
    if (handlers.empty()) {
        handlers.push_front(callback);
        return cnt_handlers++;
    }

    auto it = handlers.begin();
    std::advance(it, cnt_handlers - 1);
    handlers.insert_after(it, callback);
    return cnt_handlers++;
}

void remove_message_handler(u32 handle)
{
    if (handle == 0) {
        handlers.pop_front();
        --cnt_handlers;
        return;
    }
    auto it = handlers.begin();
    std::advance(it, handle - 1);
    handlers.erase_after(it);
    --cnt_handlers;
}

void log(const MsgInfo &info)
{
    std::time_t now = std::time(nullptr);
    std::tm tm;
    char time_str[32];
    // localtime_s(&tm, &now);
    // FIX: temp linux fix
    localtime_r(&now, &tm);
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm);

    std::string caller = info.caller;
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
    std::string file =
        info.file_path.substr(info.file_path.find_last_of("/\\") + 1);

    Log log =
        { info.level, caller, file, info.line_number, time_str, info.args };

    logs.push(log);

// #ifdef HKDEBUG
    debug_info.logs_issued++;
// #endif
}

void dispatch()
{
    Log log;
    while (logs.pop(log)) {
        for (auto &handler : handlers) {
            handler(log);
        }
    }
}

const DebugInfo& get_debug_info()
{
    return debug_info;
}

}
