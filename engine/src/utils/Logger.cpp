#include "utils/Logger.h"

#include <chrono>
#include <iomanip>

Logger *Logger::singleton = nullptr;

Logger *Logger::getInstance()
{
	if (singleton == nullptr) {
		singleton = new Logger();
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

void Logger::addMessageHandler(void *self, LoggerHandlerCallback callback)
{
	if (cntHandlers >= 5)
	{
		LOG_WARN("Currently supports only up to 5 handlers");
		return;
	}
	handlers[cntHandlers++] = {self, callback};
}

void Logger::removeMessageHandler(void *self, LoggerHandlerCallback callback)
{
	unsigned i;
	for (i = 0; i < cntHandlers; ++i) {
		if (handlers[i].self == self &&
			handlers[i].callback == callback) { break; }
	}

	memset(&handlers[i], 0, sizeof(handlers[i]));
}

void Logger::log(const MsgInfo &info)
{
	if (!cntHandlers) return;

	bool is_error = info.level < Level::LVL_WARN;
	bool is_trace = info.level == Level::LVL_TRACE;
	int log_lvl = static_cast<int>(info.level);

	// TODO: delete this
	std::time_t now = std::time(nullptr);
	std::tm tm;
	char time_str[32];
	localtime_s(&tm, &now);
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm);


	// Deleting "class " before caller's name
	std::string caller = info.callerName.substr(6);
	// Obtaining only file name instead of full path
	std::string file = info.fileName.substr(info.fileName.find_last_of("/\\") + 1) + ":";

	MsgAddInfo misc {is_error, is_trace, log_lvl, time_str, caller, file};

	for (unsigned i = 0; i < cntHandlers; ++i) {
		auto &handler = handlers[i];
		handler.callback(handler.self, info, misc);
	}
}
