#include "GuiLog.h"

#include "vendor/imgui/imgui.h"

#include "utils/Logger.h"
#include "utils/containers/hkvector.h"

#include <fstream>
#include <iomanip>

// FIX: This file is a temp solution
// Later: rewrite whole imgui log

#include "platform/Windows/WinLog.h"

struct ColorText {
    hk::vector<ImVec4> color;
    ImGuiTextBuffer buf;
    // ImVector<int> lineOffsets;
    hk::vector<int> lineOffsets;
};

static ColorText log_;

void logImGui(const Logger::MsgInfo& info, const Logger::MsgAddInfo &misc);

void addImGuiLog()
{
    log_.buf.clear();
    log_.lineOffsets.clear();
    log_.lineOffsets.push_back(0);

    // deallocWinConsole();

    Logger::getInstance()->addMessageHandler(logImGui);
}

void removeImGuiLog()
{
    Logger::getInstance()->removeMessageHandler(logImGui);
}

void logImGui(const Logger::MsgInfo& info, const Logger::MsgAddInfo &misc)
{
    // No coloring
    std::wstringstream wss;

    wss << std::left;
    wss << misc.time.c_str() << ' ';
    wss << std::setw(8)  << Logger::lookup_level[misc.log_lvl] << ' ';
    wss << std::setw(12) << misc.caller.c_str() << ' ';
    if (misc.is_trace) { wss << "---" << ' '; }

    wss << std::setw(40) << (info.args).c_str() << ' ';
    wss << std::setw(12) << (misc.is_error ? misc.file.c_str() : "");
    wss << std::setw(3)  << (misc.is_error ? info.lineNumber.c_str() : "") << ' ';
    wss << '\n';

    std::wstring wstr = wss.str();
    std::string str;
    size_t size;
    str.resize(wstr.length());
    wcstombs_s(&size, &str[0], str.size() + 1, wstr.c_str(), wstr.size());

    constexpr ImVec4
        lookup_color[static_cast<int>(Logger::Level::max_levels)] =
    {
        ImVec4(1.0f, 0.0f, 0.0f, 1.0f), // FATAL
        ImVec4(1.0f, 0.5f, 0.0f, 1.0f), // ERROR
        ImVec4(1.0f, 1.0f, 0.0f, 1.0f), // WARN
        ImVec4(0.0f, 1.0f, 0.0f, 1.0f), // INFO
        ImVec4(0.0f, 0.0f, 1.0f, 1.0f), // DEBUG
        ImVec4(0.5f, 0.5f, 0.5f, 1.0f), // TRACE
    };

    ImVec4 logColor = lookup_color[misc.log_lvl];

    u32 old_size = log_.buf.size();
    log_.buf.append(str.c_str());
    for (u32 new_size = log_.buf.size(); old_size < new_size; old_size++)
    {
        if (log_.buf[old_size] == '\n') {
            log_.lineOffsets.push_back(old_size + 1);
            log_.color.push_back(logColor);
        }
    }
}

void drawLog()
{
    ImGui::Begin("Log");
    if (ImGui::BeginChild("scrolling", ImVec2(0, 0),
                          ImGuiChildFlags_None,
                          ImGuiWindowFlags_HorizontalScrollbar))
    {
        // if (clear)
        //     Clear();
        // if (copy)
        //     ImGui::LogToClipboard();

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        const char* buf = log_.buf.begin();
        const char* buf_end = log_.buf.end();

        ImGuiListClipper clipper;
        clipper.Begin(log_.lineOffsets.size());
        while (clipper.Step()) {
            for (u32 line_no = clipper.DisplayStart;
                 line_no < static_cast<u32>(clipper.DisplayEnd); line_no++)
            {
                const char* line_start = buf + log_.lineOffsets[line_no];
                const char* line_end = (line_no + 1 < log_.lineOffsets.size()) ?
                    (buf + log_.lineOffsets[line_no + 1] - 1) : buf_end;

                ImGui::PushStyleColor(ImGuiCol_Text,
                                      (line_no < log_.color.size()) ?
                                            log_.color[line_no] : ImVec4());
                ImGui::TextUnformatted(line_start, line_end);
                ImGui::PopStyleColor();
            }
        }
        clipper.End();

        ImGui::PopStyleVar();

        // Auto scroll to the bottom
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }
    }
    ImGui::EndChild();
    ImGui::End();
}
