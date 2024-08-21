#ifndef HK_EDITOR_H
#define HK_EDITOR_H

#include "hikai.h"

class Editor final : public Application {
public:
    Editor(const AppDesc &desc)
        : Application(desc) {}

    ~Editor() {
        LOG_INFO("Editor is closed");
    }

    void init();
    void update(f32 dt);
    void render();

private:
    void processInput(f32 dt);

private:
    Window *window_ = nullptr;

    Camera camera_;

    hk::xoshiro256ss rng;
};

#endif // HK_EDITOR_H
