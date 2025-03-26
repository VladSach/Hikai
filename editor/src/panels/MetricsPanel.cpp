#include "MetricsPanel.h"

void MetricsPanel::init()
{
    is_open_ = false;
}

void MetricsPanel::deinit()
{
}

void MetricsPanel::display()
{
    if (!is_open_) { return; }

    hk::imgui::push([&](){
        if (ImGui::Begin("Metrics", &is_open_)) {

            addLogMetrics();

        } ImGui::End();
    });
}

void MetricsPanel::addLogMetrics()
{
    if (ImGui::CollapsingHeader("Logs")) {
        hk::log::DebugInfo log = hk::log::getDebugInfo();

        ImGui::Text("Logs Issued: %d", log.logsIssued);
    }
}
