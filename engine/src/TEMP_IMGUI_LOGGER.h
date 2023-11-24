/*
#include "utils/Logger.h"

class IMGUI_LOG {
public:
	struct ColorText {
        std::vector<ImVec4> color;
	    ImGuiTextBuffer buffer;
	    ImVector<int> lineOffsets;
	};

private:
	ColorText buf;

public:
	void init()
	{
    	buf.buffer.clear();
    	buf.lineOffsets.clear();
    	buf.lineOffsets.push_back(0);
	}
    ColorText* getImGuiBuf() { return &buf; }

    static void logImGui(const Logger::MsgInfo& info,
                  const Logger::MsgAddInfo &misc)
    {
	    // No coloring
	    std::wstringstream wss;

	    wss << std::left;
        wss << misc.time.c_str() << ' ';
        wss << std::setw(8)  << Logger::lookup_level[misc.log_lvl] << ' ';
	    wss << std::setw(12) << misc.caller.c_str() << ' ';
	    if (misc.is_trace) { wss << "---" << ' '; }
        wss << std::setw(40) << (info.message + " " + info.args).c_str() << ' ';
	    wss << std::setw(12) << (misc.is_error ? misc.file.c_str() : "");
	    wss << std::setw(3)  << (misc.is_error ? info.lineNumber.c_str() : "") << ' ';
	    wss << '\n';

		std::wofstream file(logFile, std::ios::out | std::ios::app);
        if (file.is_open()) {
        	file << wss.rdbuf();
            file.close();
        }

		std::wstring wstr = wss.str();
		std::string str;
		size_t size;
		str.resize(wstr.length());
		wcstombs_s(&size, &str[0], str.size() + 1, wstr.c_str(), wstr.size());

		ImVec4 logColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
		switch (info.level) {
		case Logger::Level::LVL_FATAL:
			logColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
			break;
		case Logger::Level::LVL_ERROR:
			logColor = ImVec4(1.0f, 0.5f, 0.0f, 1.0f);
			break;
		case Logger::Level::LVL_WARN:
			logColor = ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
			break;
		case Logger::Level::LVL_INFO:
			logColor = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
			break;
		case Logger::Level::LVL_DEBUG:
			logColor = ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
			break;
		case Logger::Level::LVL_TRACE:
			logColor = ImVec4(0.5f, 0.5f, 0.5f, 1.0f);
			break;
		default:
			break;
		}

        int old_size = buf.buffer.size();
        buf.buffer.append(str.c_str());
        for (int new_size = buf.buffer.size(); old_size < new_size; old_size++)
        {
            if (buf.buffer[old_size] == '\n') {
                buf.lineOffsets.push_back(old_size + 1);
                buf.color.push_back(logColor);
            }
        }
    }
};
*/
