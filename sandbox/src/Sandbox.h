#include "hikai.h"

class Sandbox final : public Application {
private:
public:
    Sandbox(const AppDesc &desc)
        : Application(desc) {}

    ~Sandbox() {
        LOG_INFO("Sandbox is closed");
    }

    void init()
    {
        // hk::Model cube = hk::loader::loadModel("");

    }

    void update(f32 dt)
    {
        // WARN: temp fix to silence warning
        // delete this when start using dt in your code
        (void)dt;
    }
};
