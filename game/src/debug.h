#ifndef HK_DEBUG_H
#define HK_DEBUG_H
// Temp test file

#include "hikai.h"

namespace hk {

inline void debugLogger()
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
}

inline void debugRingBuffer()
{
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

        // for(u32 i = 0; i < rb.size(); ++i)
        // {
        //     LOG_TRACE("rb", i, ":", rb[i].id);
        // }

        for(u32 i = 0; rb.pop(value); ++i)
        {}

        rb.push({1337, 0});
        rb.push({80085, 0});

        rb.clear();
    }
    LOG_DEBUG("=== END DEBUGGING RING BUFFER ===");
}

inline void debugEventSystem()
{
    LOG_DEBUG("=== DEBUGGING EVENT SYSTEM ===");
    {
        auto getInput = [&](hk::EventContext userdata) {
            LOG_TRACE("Key pressed: ", (char)(userdata.u32[0]));
        };

        EventSystem *evsys = EventSystem::instance();
        evsys->subscribe(hk::EVENT_KEY_PRESSED, getInput);
    }
    LOG_DEBUG("=== END DEBUGGING EVENT SYSTEM ===");
}

inline void debugInputSystem()
{
    LOG_DEBUG("=== DEBUGGING INPUT SYSTEM ===");
    {
        if (hk::input::isKeyPressed(hk::input::KEY_A))
            LOG_TRACE("Key pressed:",
                      hk::input::getKeycodeStr(hk::input::KEY_A));

        // if (hk::input::isKeyDown(hk::input::KEY_A))
        //     LOG_TRACE("Key down:",
        //               hk::input::getKeycodeStr(hk::input::KEY_A));

        if (hk::input::isKeyReleased(hk::input::KEY_A))
            LOG_TRACE("Key released:",
                      hk::input::getKeycodeStr(hk::input::KEY_A));


        if(hk::input::isMousePressed(hk::input::BUTTON_LEFT))
            LOG_TRACE("Left button clicked");

        // if(hk::input::isMouseDown(hk::input::BUTTON_LEFT))
        //     LOG_TRACE("Left button down");

        if(hk::input::isMouseReleased(hk::input::BUTTON_LEFT))
            LOG_TRACE("Left button released");

        i32 mouseX, mouseY;
        hk::input::getMousePosition(mouseX, mouseY);
        // LOG_DEBUG("Mouse current pos:", mouseX, mouseY);

        hk::input::getMouseDelta(mouseX, mouseY);
        // LOG_DEBUG("Mouse delta pos:", mouseX, mouseY);

        if (hk::input::getMouseWheelDelta() > 0) {
            LOG_DEBUG("Mouse wheel went up");
        } else if (hk::input::getMouseWheelDelta() < 0) {
            LOG_DEBUG("Mouse wheel went down");
        } else {
            // LOG_DEBUG("Mouse wheel unchanged");
        }
    }
    LOG_DEBUG("=== END DEBUGGING INPUT SYSTEM ===");
}

};
#endif // HK_DEBUG_H
