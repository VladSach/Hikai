#ifndef HK_INPUT_H
#define HK_INPUT_H

#include "defines.h"
#include "EventSystem.h"

namespace hk::input {

enum Button : u16;

// Returns true if the key was pressed once
HKAPI b8 isKeyPressed(Button button);
// Returns true if the key being pressed down
HKAPI b8 isKeyDown(Button button);
// Returns true if the key was just released
HKAPI b8 isKeyReleased(Button button);

// Returns true if the mouse button was pressed once
HKAPI b8 isMousePressed(Button button);
// Returns true if the mouse button being pressed down
HKAPI b8 isMouseDown(Button button);
// Returns true if the mouse button was just released
HKAPI b8 isMouseReleased(Button button);

HKAPI void getMousePosition(i32 &x, i32 &y);
HKAPI void getMouseDelta(i32 &x, i32 &y);

// Returns [-1, 0, 1] depending on wheel scroll direction
HKAPI i16 getMouseWheelDelta();

HKAPI const char* getKeycodeStr(Button button);

void init();
void deinit();
void update();

void registerKeyPress(EventContext keyinfo);

void registerMouseMove(EventContext mouseinfo);
void registerMousePress(EventContext mouseinfo);
void registerMouseWheel(EventContext mouseinfo);

#ifdef KEY_EXECUTE // If defined in Windows
    #undef KEY_EXECUTE
#endif // KEY_EXECUTE
//
enum Button : u16 {
    /* Keys are lined up with Win Virtual Keys -> ASCII for letters and numbers
     *https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
     */

    EMPTY_KEY = 0, // For Debug only

    // Mouse buttons 1 - 4
    BUTTON_LEFT = 0x01,
    BUTTON_RIGHT = 0x02,
    BUTTON_MIDDLE = 0x04,

    MAX_BUTTONS, // Can add up to 7 buttons

    // Keyboard keys 8 - 222
    KEY_BACKSPACE = 0x08,
    KEY_TAB       = 0x09,
    KEY_ENTER     = 0x0D,

    KEY_SHIFT     = 0x10,
    KEY_CTRL      = 0x11,
    KEY_ALT       = 0x12, // VK_MENU
    KEY_PAUSE     = 0x13,
    KEY_CAPITAL   = 0x14,

    KEY_ESC       = 0x1B,
    KEY_SPACE     = 0x20,

    KEY_PAGEUP    = 0x21,
    KEY_PAGEDOWN  = 0x22,
    KEY_END       = 0x23,
    KEY_HOME      = 0x24,
    KEY_LEFT      = 0x25,
    KEY_UP        = 0x26,
    KEY_RIGHT     = 0x27,
    KEY_DOWN      = 0x28,
    KEY_SELECT    = 0x29,
    KEY_PRINT     = 0x2A,
    KEY_EXECUTE   = 0x2B,
    KEY_SNAPSHOT  = 0x2C, // Print screen
    KEY_INSERT    = 0x2D,
    KEY_DELETE    = 0x2E,
    KEY_HELP      = 0x2F,

    KEY_0 = 0x30,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,

    KEY_A = 0x41,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,

    KEY_LSUPER = 0x5B,
    KEY_RSUPER = 0x5C,
    KEY_APPS   = 0x5D,

    KEY_SLEEP  = 0x5F,

    KEY_NUMPAD0 = 0x60,
    KEY_NUMPAD1,
    KEY_NUMPAD2,
    KEY_NUMPAD3,
    KEY_NUMPAD4,
    KEY_NUMPAD5,
    KEY_NUMPAD6,
    KEY_NUMPAD7,
    KEY_NUMPAD8,
    KEY_NUMPAD9,

    KEY_MULTIPLY  = 0x6A,
    KEY_ADD       = 0x6B,
    KEY_SEPARATOR = 0x6C,
    KEY_SUBTRACT  = 0x6D,
    KEY_DECIMAL   = 0x6E,
    KEY_DIVIDE    = 0x6F,

    KEY_F1 = 0x70,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_F13,
    KEY_F14,
    KEY_F15,
    KEY_F16,
    KEY_F17,
    KEY_F18,
    KEY_F19,
    KEY_F20,
    KEY_F21,
    KEY_F22,
    KEY_F23,
    KEY_F24,

    KEY_NUMLOCK = 0x90,
    KEY_SCROLL  = 0x91,

    KEY_LSHIFT  = 0xA0,
    KEY_RSHIFT  = 0xA1,
    KEY_LCTRL   = 0xA2,
    KEY_RCTRL   = 0xA3,
    KEY_LALT    = 0xA4,
    KEY_RALT    = 0xA5,

    KEY_COLON  = 0xBA,
    KEY_EQUAL  = 0xBB,
    KEY_COMMA  = 0xBC,
    KEY_MINUS  = 0xBD,
    KEY_PERIOD = 0xBE,
    KEY_SLASH  = 0xBF,
    KEY_TILDE  = 0xC0,

    KEY_LBRACKET  = 0xDB,
    KEY_BACKSLASH = 0xDC,
    KEY_RBRACKET  = 0xDD,

    KEY_QUOTE = 0xDE,

    MAX_KEYS,

    // Some aliases
    KEY_SEMICOLON = KEY_COLON,
    KEY_PLUS = KEY_EQUAL,
    KEY_UNDERSCORE = KEY_MINUS,
    KEY_QUESTIONMARK = KEY_SLASH,
    KEY_GRAVE = KEY_TILDE,
    KEY_LBRACE = KEY_LBRACKET,
    KEY_RBRACE = KEY_RBRACKET,
    KEY_PIPE = KEY_BACKSLASH,
};
};


#endif // HK_INPUT_H
