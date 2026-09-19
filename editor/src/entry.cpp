#include "entry.h"

#include "Editor.h"

void assert_handler(const char *title, const char *reason, const char *va)
{
    // LOG_FATAL(title, reason, va);
    hk::platform::create_task_dialog(title, reason, va);
}
hk_assert_callback hk_assert_handler = assert_handler;

// FIX: temp place
// Maybe should put it in utils like to_string
#include "platform/console/console.h"
#include "utils/to_string.h"
#include <iomanip>

constexpr u32 max_func_name_length = 24;

void log_console(const hk::log::Log &log)
{
    /* Colored output
     * time [log_lvl]: caller message [opt]: args file line
     * gray diff cyan white white red red
     * More info about colors: https://ss64.com/nt/syntax-ansi.html
     * */

    constexpr char const *lookup_color[] =
    {
        "0;41m",
        "1;31m",
        "1;33m",
        "1;32m",
        "1;34m",
        "1;30m"
    };

    b8 is_error = log.level  < hk::log::Level::LVL_WARN;
    b8 is_trace = log.level == hk::log::Level::LVL_TRACE;

    std::stringstream wss;
    wss << std::left;
    wss << "\033[1;90m" << log.time.c_str() << "\033[0;10m" << ' ';

    wss << "\033[" << lookup_color[static_cast<u32>(log.level)]
        << std::setw(8) << to_string(log.level)
        << "\033[0;10m" << ' ';

    std::string caller = log.caller;
    if (caller.size() > max_func_name_length) {
        caller.erase(max_func_name_length - 3, std::string::npos);
        caller.append("...");
    }

    wss << "\033[1;36m"
        << std::setw(max_func_name_length) << caller.c_str()
        << "\033[0;10m" << ' ';

    if (is_trace) {
        wss << "\033[1;97m"
            <<  "---" << ' '
            << "\033[0;10m";
    }

    // Max message length is 160 chars
    wss << "\033[1;97m";
        std::string message = log.args;
        u64 msg_length = message.length();
        if (msg_length > 85) {
            u64 pos = 0;
            std::string row, token;
            std::string delimiter = " ";
            while ((pos = message.find(delimiter)) != std::string::npos) {
                token = message.substr(0, pos);
                if (row.length() + token.length() >= 150) {
                    wss << "\n   + " << row.c_str();
                    row.clear();
                }
                row.append(token);
                row.push_back(' ');
                message.erase(0, pos + delimiter.length());
            }
            wss << "\n   + " << row.c_str() << message.c_str();
        } else {
            wss << message.c_str();
        }
    wss << "\033[0;10m";

    wss << "\033[1;31m"
        << (msg_length > 85 ? "\n   -> " : " ")
        << (is_error ? log.file.c_str() : "") << ' '
        << (is_error ? log.line.c_str() : "")
    << "\033[0;10m";

    wss << '\n';

    // u32 length = static_cast<u32>(wss.str().size());
    hk::platform::write_console(wss.str().c_str());
}

void log_file(const hk::log::Log &log)
{
    // TODO: test and fix log to file
    // No coloring
    std::wstringstream wss;
    wss << std::left;
    wss << log.time.c_str() << ' ';
    wss << std::setw(8) << to_string(log.level) << ' ';
    std::string caller = log.caller;
    if (caller.size() > max_func_name_length) {
        caller.erase(max_func_name_length - 3, std::string::npos);
        caller.append("...");
    }
    wss << std::setw(max_func_name_length) << caller.c_str() << ' ';
    // if (is_trace) { wss << "---" << ' '; }
    // wss << std::setw(40) << (info.message + " " + info.args).c_str() << ' ';
    wss << std::setw(30) << (log.args).c_str() << ' ';

    b8 is_error = log.level  < hk::log::Level::LVL_WARN;
    b8 is_trace = log.level == hk::log::Level::LVL_TRACE;
    wss << std::setw(12) << (is_error ? log.file.c_str() : "");
    wss << std::setw(3)  << (is_trace ? log.line.c_str() : "") << ' ';
    wss << '\n';

    hk::string log_file = "";
    if (log_file.empty()) { return; }

    // std::wofstream file(log_file, std::ios::app);
    // if (file.is_open()) {
    //     file << wss.rdbuf();
    //     file.close();
    // }

    // TODO:
    // hk::platform::write_file(wss.str().c_str());
}

Application* create_app()
{
    AppDesc desc;
    desc.title = "Hikai Editor";

    u32 hndl_console = add_message_handler(log_console);
    // u32 hndl_file = add_message_handler(log_file);

    hk_set_assert_handler(assert_handler);

    return new Editor(desc);
}
