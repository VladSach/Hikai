#ifndef HK_EVENTS_H
#define HK_EVENTS_H

#include "hkcommon.h"
#include "hkstl/containers/hkvector.h"
#include "hkstl/containers/hkring_buffer.h"

#include <functional>
#include <unordered_map>

namespace hk::event {

struct EventContext {
    union {
        u16 u16[4];
        i16 i16[4];

        u32 u32[2];
        i32 i32[2];

        f32 f32[2];

        u64 u64;
    };
};

// using EventCallback = void(*)(EventContext userdata);
using EventCallback =
    std::function<void(const EventContext &userdata, void *listener)>;

struct Event {
    void *sender;
    u32 code;
    EventContext userdata;
};

struct Subscriber {
    void *listener;
    EventCallback callback;
};

HKAPI b8 subscribe(u32 code, const EventCallback &callback,
                   void *listener = nullptr);

HKAPI b8 unsubscribe(u32 code, const EventCallback &callback,
                     void *listener = nullptr);

HKAPI b8 fire(u32 code, const EventContext &userdata,
              void *sender = nullptr);

void init();
void deinit();
void dispatch();

// INFO: Codes from 0 to 100 reserved by Hikai
enum EventCode : u32 {
    EVENT_EMPTY = 0,

    // u64 = hk::ErrorCode
    EVENT_APP_SHUTDOWN,

    // u16[0] = key, u16[1] = isPressed
    EVENT_KEY_PRESSED,
    EVENT_KEY_RELEASED,

    // u16[0] = button, u16[1] = isPressed
    EVENT_MOUSE_PRESSED,
    EVENT_MOUSE_RELEASED,

    // i32[0] = x, i32[1] = y
    EVENT_MOUSE_MOVED,

    // i32[0] = delta x, i32[1] = delta y
    EVENT_RAW_MOUSE_MOVED,

    // i16[0] = delta
    EVENT_MOUSE_WHEEL,

    // u32[0] = width, u32[1] = height
    EVENT_WINDOW_RESIZE,

    // u32[0] = handle, u32[1] = asset type
    EVENT_ASSET_LOADED,

    // u32[0] = handle, u32[1] = asset type
    // EVENT_ASSET_MODIFIED,

    // u32[0] = handle
    EVENT_MATERIAL_MODIFIED,

    MAX_EVENT_CODES
};

// INFO: Codes from 0 to 100 reserved by Hikai
enum ErrorCode : u64 {
    ERROR_UNKNOWN = 0,

    ERROR_UNSUPPORTED_GRAPHICS_API,

    ERROR_VK_VALIDATION_LAYER,

    MAX_ERROR_CODES
};

}

#endif // HK_EVENTS_H
