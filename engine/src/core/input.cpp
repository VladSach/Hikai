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

    EventSystem &evsys = *EventSystem::instance();
    evsys.subscribe(EVENT_KEY_PRESSED, registerKeyPress);
    evsys.subscribe(EVENT_KEY_RELEASED, registerKeyPress);

    evsys.subscribe(EVENT_MOUSE_MOVED,     registerMouseMove);
    evsys.subscribe(EVENT_RAW_MOUSE_MOVED, registerRawMouseMove);
    evsys.subscribe(EVENT_MOUSE_PRESSED,   registerMousePress);
    evsys.subscribe(EVENT_MOUSE_RELEASED,  registerMousePress);
    evsys.subscribe(EVENT_MOUSE_WHEEL,     registerMouseWheel);

    LOG_INFO("Input Subsystem initialized");
}

void deinit()
{
    if (!initialized) { return; }

    EventSystem &evsys = *EventSystem::instance();
    evsys.unsubscribe(EVENT_KEY_PRESSED, registerKeyPress);
    evsys.unsubscribe(EVENT_KEY_RELEASED, registerKeyPress);

    evsys.unsubscribe(EVENT_MOUSE_MOVED, registerMouseMove);
    evsys.unsubscribe(EVENT_MOUSE_PRESSED, registerMousePress);
    evsys.unsubscribe(EVENT_MOUSE_RELEASED, registerMousePress);
    evsys.unsubscribe(EVENT_MOUSE_WHEEL, registerMouseWheel);

    LOG_INFO("Input Subsystem deinitialized");
}

void registerKeyPress(hk::EventContext keyinfo)
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

void registerMouseMove(EventContext mouseinfo)
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

void registerRawMouseMove(EventContext mouseinfo)
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

void registerMousePress(EventContext mouseinfo)
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

void registerMouseWheel(EventContext mouseinfo)
{
    if (!initialized) {
        LOG_WARN("Input Subsystem was not initialized");
        return;
    }

    mouse.z_delta = mouseinfo.i16[0];

    // LOG_DEBUG("Mouse wheel:", mouse.z_delta);
}

const char* getKeycodeStr(Button button)
{
    constexpr char const *NDF = "Undefined";
    constexpr char const *lookup_keys[MAX_KEYS] = {
        "Empty",

        "Mouse Left",
        "Mouse Right",
        NDF, // 0x03
        "Mouse Middle",

        // 0x05 - 0x07
        NDF, NDF, NDF,

        "Backspace",
        "Tab",

        // 0x0A - 0x0C
        NDF, NDF, NDF,

        "Enter",

        // 0x0E - 0x0F
        NDF, NDF,

        "Shift",
        "Ctrl",
        "Alt",
        "Pause",
        "Caps Lock",

        // 0x15 - 0x1A
        NDF, NDF, NDF, NDF, NDF, NDF,

        "Esc",

        // 0x1C - 0x1F
        NDF, NDF, NDF, NDF,

        "Space",
        "Page Up",
        "Page Down",
        "End",
        "Home",
        "Left",
        "Up",
        "Right",
        "Down",
        "Select",
        "Print",
        "Execute",
        "Print Screen",
        "Insert",
        "Delete",
        "Help",

        "0", "1", "2", "3", "4",
        "5", "6", "7", "8", "9",

        // 0x3A - 0x40
        NDF, NDF, NDF,
        NDF, NDF, NDF, NDF,

        "A", "B", "C", "D", "E", "F",
        "G", "H", "I", "J", "K", "L",
        "M", "N", "O", "P", "Q", "R",
        "S", "T", "U", "V", "W", "X",
        "Y", "Z",

        "Left Super",
        "Right Super",
        "Apps",

        // 0x5E
        NDF,

        "Sleep",

        "Numpad 0",
        "Numpad 1",
        "Numpad 2",
        "Numpad 3",
        "Numpad 4",
        "Numpad 5",
        "Numpad 6",
        "Numpad 7",
        "Numpad 8",
        "Numpad 9",

        "Numpad Mult",
        "Numpad Add",
        "Numpad Sep",
        "Numpad Sub",
        "Numpad Dec",
        "Numpad Div",

        "F1", "F2", "F3", "F4", "F5",
        "F6", "F7", "F7", "F8", "F9",
        "F10", "F11", "F12", "F13", "F14",
        "F15", "F16", "F17", "F18", "F19",
        "F20", "F21", "F22", "F23", "F24",

        // 0x88 - 0x8F
        NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF,

        "Num Lock",
        "Scroll Lock",

        // 0x92 - 0x9F
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF,

        "Left Shift",
        "Right Shift",
        "Left Ctrl",
        "Right Ctrl",
        "Left Alt",
        "Right Alt",

        // 0xA6 - 0xB9
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,

        ":;",
        "=+",
        ",",
        "-_",
        ".",
        "/?",
        "~`",

        // 0xC1 - 0xDA
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,
        NDF, NDF, NDF, NDF, NDF,
        NDF,

        "[{",
        "\\|",
        "}]",

        "\'\"",
    };
    return lookup_keys[button];
}
}
