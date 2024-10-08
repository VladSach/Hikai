#include "LogPanel.h"

#include "vendor/imgui/imgui.h"

#include "utils/Logger.h"
#include "utils/containers/hkvector.h"

#include <fstream>
#include <iomanip>

// FIX: This file is a temp solution
// Later: rewrite whole imgui log

#include "platform/Windows/WinLog.h"

void LogPanel::init(GUI *gui)
{
    this->gui = gui;

    handle = hk::log::addMessageHandler([&](const hk::log::Log &log){
        addLog(log);
    });

    deallocWinConsole();
}

void LogPanel::deinit()
{
    hk::log::removeMessageHandler(handle);
}

void LogPanel::addLog(const hk::log::Log &log)
{
    buf.push_back(log);
}


void LogPanel::display()
{
    gui->pushCallback([&](){
        ImGui::Begin("Log");

        addControlPanel();
        addLogsPanel();

        ImGui::End();
    });
}

void LogPanel::addControlPanel()
{
    f32 height = ImGui::GetContentRegionAvail().y * .0375f;

    ImGui::BeginChild("Log Control Panel", ImVec2(0, height),
                      // ImGuiChildFlags_Border  |
                      // ImGuiChildFlags_ResizeY |
                      ImGuiWindowFlags_NoScrollbar |
                      ImGuiChildFlags_None);

    char buffer[256] = {};
    strcpy_s(buffer, search.c_str());
    if (ImGui::InputTextWithHint("##LogSearchArea", "Search", buffer, 256)) {
        search = buffer;
    }

    ImGui::SameLine();

    constexpr char const *severity_lookup[] =
    {
        "Fatal ",
        "Error ",
        "Warn  ",
        "Info  ",
        "Debug ",
        "Trace "
    };

    if (ImGui::Button("Filter"))
        ImGui::OpenPopup("LogFilterPopup");
    if (ImGui::BeginPopup("LogFilterPopup")) {
        ImGui::PushItemFlag(ImGuiItemFlags_AutoClosePopups, false);

        for (u32 i = 0; i < static_cast<u32>(hk::log::Level::MAX_LVL); ++i) {
            if (ImGui::MenuItem(severity_lookup[i], "", (severities & (1 << i)))) {
                severities ^= (1 << i);
            }
        }
        ImGui::PopItemFlag();

        ImGui::EndPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear")) {
        buf.clear();
    }

    ImGui::EndChild();
}

void LogPanel::addLogsPanel()
{
    constexpr ImVec4 lookup_color[] =
    {
        ImVec4(1.0f, 0.0f, 0.0f, 1.0f), // FATAL
        ImVec4(1.0f, 0.5f, 0.0f, 1.0f), // ERROR
        ImVec4(1.0f, 1.0f, 0.0f, 1.0f), // WARN
        ImVec4(0.0f, 1.0f, 0.0f, 1.0f), // INFO
        ImVec4(0.0f, 0.0f, 1.0f, 1.0f), // DEBUG
        ImVec4(0.5f, 0.5f, 0.5f, 1.0f), // TRACE
    };

    ImGuiTableFlags flags =
        ImGuiTableFlags_Resizable                  |
        ImGuiTableFlags_Reorderable                |
        ImGuiTableFlags_Hideable                   |
        ImGuiTableFlags_HighlightHoveredColumn     |
        ImGuiTableFlags_ScrollY                    |
        ImGuiTableFlags_ScrollX                    |
        ImGuiTableFlags_BordersV                   |
        ImGuiTableFlags_RowBg                      |
        ImGuiTableFlags_None;

    if (ImGui::BeginTable("logs", 6, flags)) {
        ImGuiTable *table = ImGui::GetCurrentContext()->CurrentTable;

        ImGui::TableSetupColumn("Severity");
        ImGui::TableSetupColumn("Caller");
        ImGui::TableSetupColumn("File");
        ImGui::TableSetupColumn("Line");
        ImGui::TableSetupColumn("Time");
        ImGui::TableSetupColumn("Message");

        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableHeadersRow();

        for (u32 row = 0; row < buf.size(); ++row) {
            hk::log::Log log = buf.at(row);

            if (severities && !(severities & (1 << (static_cast<u32>(log.level))))) {
                continue;
            }

            if (search.length() && !(
                log.caller.find(search) != std::string::npos ||
                log.file.find(search)   != std::string::npos ||
                log.line.find(search)   != std::string::npos ||
                log.time.find(search)   != std::string::npos ||
                log.args.find(search)   != std::string::npos)
            ) {
                continue;
            }

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(lookup_color[static_cast<u32>(log.level)],
                                "%s", levelToString(log.level).c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", log.caller.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", log.file.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", log.line.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", log.time.c_str());

            ImGui::TableNextColumn();
            ImGui::Text("%s", log.args.c_str());

            i32 idx = ImGui::TableGetHoveredRow() - 1;
            if (idx == row) {
                table->RowBgColor[1] = ImGui::GetColorU32(ImGuiCol_Border);
            }
        }

        /* PERF: Has issues, shows hovered row for previous frame
            * https://github.com/ocornut/imgui/issues/6588 */
        static i32 hovered = -1;
        i32 idx = ImGui::TableGetHoveredRow();
        std::string copy;
        if ((idx > 0) && ImGui::IsMouseReleased(1)) {
            ImGui::OpenPopup("MessageColumnPopup", ImGuiPopupFlags_NoOpenOverExistingPopup);
            hovered = idx;
        }
        if (ImGui::BeginPopup("MessageColumnPopup")) {
            if (ImGui::MenuItem("Copy Message")) {
                hk::platform::copyToClipboard(buf.at(hovered - 1).args);
                ImGui::CloseCurrentPopup();
                hovered = -1;
            }

            ImGui::EndPopup();
        }

        // Auto scroll to the bottom
        if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
            ImGui::SetScrollHereY(1.0f);
        }

        ImGui::EndTable();
    }

}
