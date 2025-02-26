#ifndef HK_SETTINGS_PANEL_H
#define HK_SETTINGS_PANEL_H

#include "hikai.h"

class SettingsPanel {
public:
    void init(Renderer *renderer);
    void deinit();

    void display();

private:
    void addInstanceSettings();
    void addSwapchainSettings();

    void addShaderSettings();

public:
    b8 is_open_;

    b8 enable_post_process_;

private:
    Renderer *renderer_;
};

#endif // HK_SETTINGS_PANEL_H
