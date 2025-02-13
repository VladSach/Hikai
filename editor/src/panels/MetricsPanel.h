#ifndef HK_METRICS_PANEL_H
#define HK_METRICS_PANEL_H

#include "hikai.h"

class MetricsPanel {
public:
    void init();
    void deinit();

    void display();

private:
    void addLogMetrics();
    void addMonitorInfo();
    void addCPUInfo();
    void addGPUInfo();

public:
    b8 is_open_;
};

#endif // HK_METRICS_PANEL_H
