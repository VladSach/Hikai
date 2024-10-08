#ifndef HK_LOG_PANEL_H
#define HK_LOG_PANEL_H

#include "hikai.h"

class LogPanel {
public:
    void init(GUI *gui);
    void deinit();

    void display();

private:
    void addLog(const hk::log::Log &log);

    void addControlPanel();
    void addLogsPanel();

private:
    GUI *gui;

    u32 handle;
    hk::vector<hk::log::Log> buf;

    // Control Panel
    std::string search;
    i32 severities = 0;

};

#endif // HK_LOG_PANEL_H
