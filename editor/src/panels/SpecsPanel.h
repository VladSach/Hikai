#ifndef HK_SPECS_PANEL_H
#define HK_SPECS_PANEL_H

#include "hikai.h"

class SpecsPanel {
public:
    void init();
    void deinit();

    void display();

private:
    void addMonitorSpec();
    void addCPUSpec();
    void addGPUSpec();

public:
    b8 is_open_;
};

#endif // HK_SPECS_PANEL_H
