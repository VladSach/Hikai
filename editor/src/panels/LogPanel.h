#ifndef HK_LOG_PANEL_H
#define HK_LOG_PANEL_H

#include "hikai.h"

class LogPanel {
public:
    void init();
    void deinit();

    void display();

private:
    void addControlPanel();
    void addLogsPanel();

public:
    b8 is_open_;

private:
    u32 handle;
    hk::vector<hk::log::Log> buf;

    // Control Panel
    std::string search;
    u8 severities; // Bit field
};

#endif // HK_LOG_PANEL_H
