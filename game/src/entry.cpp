#include "entry.h"

#include "hikai.h"

class Blight : public Application {
public:
    Blight(const AppDesc &desc)
        : Application(desc) {}

    void init()
    {
        LOG_DEBUG("=== DEBUGGING LOGGER ===");
        {
            LOG_INFO("Test");
            LOG_WARN("Test");
            LOG_ERROR("Test test test test test test test test test");
            LOG_FATAL("Test test test test test test test test test");

            LOG_ERROR("Really long string",
                      "Really long string",
                      "Really long string",
                      "Really long string",
                      "Really long string",
                      "Really long string",
                      "Really long string");

            LOG_INFO( "Another Really long string",
                      "Another Really long string",
                      "Another Really long string",
                      "Another Really long string",
                      "Another Really long string",
                      "Another Really long string",
                      "Another Really long string",
                      "Another Really long string",
                      "Another Really long string",
                      "Another Really long string",
                      "Another Really long string",
                      "Another Really long string",
                      "Another Really long string");
        }
        LOG_DEBUG("=== END DEBUGGING LOGGER ===");

        LOG_DEBUG("=== DEBUGGING RING BUFFER ===");
        {
            struct DebStr {
                u32 id;
                f32 dummy;
            };

            // hk::ring_buffer<u32, 10, true> rb;
            hk::ring_buffer<DebStr, 10> rb;

            for(u32 i = 0; rb.push({i, 0}) && i < 15; ++i)
            {}

            DebStr value; rb.peek(value);
            LOG_TRACE("rb head at:",  value.id);

            for(u32 i = 0; i < rb.getSize(); ++i)
            {
                LOG_TRACE("rb", i, ":", rb[i].id);
            }

            for(u32 i = 0; rb.pop(value); ++i)
            {}

            rb.push({1337, 0});
            rb.push({80085, 0});

            rb.clear();
        }
        LOG_DEBUG("=== END DEBUGGING RING BUFFER ===");


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

Application* create_app()
{
    AppDesc desc;
    desc.title = L"Blight";

    return new Blight(desc);
}
