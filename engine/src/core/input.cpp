#include "input.h"

namespace hk::input {

enum ButtonState {
    JUST_PRESSED,
    JUST_RELEASED,
    STILL_PRESSED,
    STILL_RELEASED,

    MAX_BUTTON_STATES
};

struct KeyboardState {
    b8 keys[MAX_KEYS] = {};
    b8 keys_prev[MAX_KEYS] = {};
};

struct MouseState {
    i32 x = 0; i32 y = 0;
    i16 z_delta = 0;

    i32 x_delta = 0;
    i32 y_delta = 0;
    b8 buttons[MAX_BUTTONS] = {};
    b8 buttons_prev[MAX_BUTTONS] = {};
};

static KeyboardState keyboard;
static MouseState mouse;
static b8 initialized = false;

void update()
{
    mouse.z_delta = 0;

    mouse.x_delta = 0;
    mouse.y_delta = 0;

    memcpy(mouse.buttons_prev, mouse.buttons, sizeof(mouse.buttons));
    memcpy(keyboard.keys_prev, keyboard.keys, sizeof(keyboard.keys));
}

ButtonState getKeyState(Button button)
{
    if (keyboard.keys_prev[button]) {
        return (keyboard.keys[button]) ? STILL_PRESSED : JUST_RELEASED;
    }

    return (keyboard.keys[button]) ? JUST_PRESSED : STILL_RELEASED;
}

ButtonState getMouseState(Button button)
{
    if (mouse.buttons_prev[button]) {
        return (mouse.buttons[button]) ? STILL_PRESSED : JUST_RELEASED;
    }

    return (mouse.buttons[button]) ? JUST_PRESSED : STILL_RELEASED;
}

b8 isKeyPressed(Button button)
{
    if (!initialized) {
        LOG_WARN("Input system was not initialized");
        return false;
    }

    return (getKeyState(button) == JUST_PRESSED);
}

b8 isKeyDown(Button button)
{
    if (!initialized) {
        LOG_WARN("Input system was not initialized");
        return false;
    }

    return (getKeyState(button) == JUST_PRESSED ||
            getKeyState(button) == STILL_PRESSED);
}

b8 isKeyReleased(Button button)
{
    if (!initialized) {
        LOG_WARN("Input system was not initialized");
        return false;
    }

    return (getKeyState(button) == JUST_RELEASED);
}

b8 isMousePressed(Button button)
{
    if (!initialized) {
        LOG_WARN("Input system was not initialized");
        return false;
    }

    return (getMouseState(button) == JUST_PRESSED);
}

b8 isMouseDown(Button button)
{
    if (!initialized) {
        LOG_WARN("Input system was not initialized");
        return false;
    }

    return (getMouseState(button) == JUST_PRESSED ||
            getMouseState(button) == STILL_PRESSED);
}

b8 isMouseReleased(Button button)
{
    if (!initialized) {
        LOG_WARN("Input system was not initialized");
        return false;
    }

    return (getMouseState(button) == JUST_RELEASED);
}

void getMousePosition(i32 &x, i32 &y)
{
    if (!initialized) {
        LOG_WARN("Input system was not initialized");
        return;
    }

    x = mouse.x;
    y = mouse.y;
}

void getMouseDelta(i32 &x, i32 &y)
{
    if (!initialized) {
        LOG_WARN("Input system was not initialized");
        return;
    }

    x = mouse.x_delta;
    y = mouse.y_delta;
}

i16 getMouseWheelDelta()
{
    return mouse.z_delta;
}

void init()
{
    initialized = true;

    hk::event::subscribe(hk::event::EVENT_KEY_PRESSED, registerKeyPress);
    hk::event::subscribe(hk::event::EVENT_KEY_RELEASED, registerKeyPress);

    hk::event::subscribe(hk::event::EVENT_MOUSE_MOVED,     registerMouseMove);
    hk::event::subscribe(hk::event::EVENT_RAW_MOUSE_MOVED, registerRawMouseMove);
    hk::event::subscribe(hk::event::EVENT_MOUSE_PRESSED,   registerMousePress);
    hk::event::subscribe(hk::event::EVENT_MOUSE_RELEASED,  registerMousePress);
    hk::event::subscribe(hk::event::EVENT_MOUSE_WHEEL,     registerMouseWheel);

    LOG_INFO("Input Subsystem initialized");
}

void deinit()
{
    if (!initialized) { return; }

    hk::event::unsubscribe(hk::event::EVENT_KEY_PRESSED,  registerKeyPress);
    hk::event::unsubscribe(hk::event::EVENT_KEY_RELEASED, registerKeyPress);

    hk::event::unsubscribe(hk::event::EVENT_MOUSE_MOVED,     registerMouseMove);
    hk::event::unsubscribe(hk::event::EVENT_RAW_MOUSE_MOVED, registerRawMouseMove);
    hk::event::unsubscribe(hk::event::EVENT_MOUSE_PRESSED,   registerMousePress);
    hk::event::unsubscribe(hk::event::EVENT_MOUSE_RELEASED,  registerMousePress);
    hk::event::unsubscribe(hk::event::EVENT_MOUSE_WHEEL,     registerMouseWheel);

    LOG_INFO("Input Subsystem deinitialized");
}

void registerKeyPress(const hk::event::EventContext &keyinfo, void*)
{
    if (!initialized) {
        LOG_WARN("Input Subsystem was not initialized");
        return;
    }

    Button button = static_cast<Button>(keyinfo.u16[0]);
    b8 pressed = static_cast<b8>(keyinfo.u16[1]);

    // LOG_DEBUG("Key pressed:", getKeycodeStr(button));

    if (keyboard.keys[button] == pressed) { return; }

    keyboard.keys[button] = pressed;
}

void registerMouseMove(const hk::event::EventContext &mouseinfo, void*)
{
    if (!initialized) {
        LOG_WARN("Input Subsystem was not initialized");
        return;
    }

    i32 x = mouseinfo.i32[0];
    i32 y = mouseinfo.i32[1];

    if (mouse.x == x && mouse.y == y) { return; }

    // LOG_DEBUG("Mouse current pos:", x, y);
    mouse.x_delta = x - mouse.x;
    mouse.y_delta = y - mouse.y;
    mouse.x = x;
    mouse.y = y;
}

void registerRawMouseMove(const hk::event::EventContext &mouseinfo, void*)
{
    if (!initialized) {
        LOG_WARN("Input Subsystem was not initialized");
        return;
    }

    i32 x_delta = mouseinfo.i32[0];
    i32 y_delta = mouseinfo.i32[1];

    mouse.x_delta = x_delta;
    mouse.y_delta = y_delta;
    // mouse.x += x_delta;
    // mouse.y += y_delta;
}

void registerMousePress(const hk::event::EventContext &mouseinfo, void*)
{
    if (!initialized) {
        LOG_WARN("Input Subsystem was not initialized");
        return;
    }

    Button button = static_cast<Button>(mouseinfo.u16[0]);
    b8 pressed = static_cast<b8>(mouseinfo.u16[1]);

    // LOG_DEBUG("Mouse button pressed:", getKeycodeStr(button));

    if (mouse.buttons[button] == pressed) { return; }

    mouse.buttons[button] = pressed;
}

void registerMouseWheel(const hk::event::EventContext &mouseinfo, void*)
{
    if (!initialized) {
        LOG_WARN("Input Subsystem was not initialized");
        return;
    }

    mouse.z_delta = mouseinfo.i16[0];

    // LOG_DEBUG("Mouse wheel:", mouse.z_delta);
}

}
