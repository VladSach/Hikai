// FIX: MOVE TO SEPARATE FILE
// CODE FROM HERE
#include "hikai.h"
#include "debug.h" // Temp test file

class Blight : public Application {
private:
public:
    Blight(const AppDesc &desc)
        : Application(desc) {}

    void init()
    {

    }

    void update(f32 dt)
    {
        // WARN: temp fix to silence warning
        // delete this when start using dt in your code
        (void)dt;

    }

    void render()
    {

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

    return new Blight(desc);
}
