// FIX: MOVE TO SEPARATE FILE
// CODE FROM HERE
#include "hikai.h"
#include "debug.h" // Temp test file

class Blight final : public Application {
private:
public:
    Blight(const AppDesc &desc)
        : Application(desc) {}

    ~Blight() {
        LOG_INFO("Blight is closed");
    }

    void init()
    {

    }

    void update(f32 dt)
    {
        // WARN: temp fix to silence warning
        // delete this when start using dt in your code
        (void)dt;
    }
};
// FIX: TO HERE

#include "entry.h"

Application* create_app()
{
    AppDesc desc;
    desc.title = L"Blight";
    desc.width = 1020;
    desc.height = 780;
    desc.window = new WinWindow();

    return new Blight(desc);
}
