#ifndef HK_EVENTSYSTEM_H
#define HK_EVENTSYSTEM_H

#include "defines.h"
#include "utils/containers/hkvector.h"
#include "utils/containers/hkring_buffer.h"

#include <functional>
// TODO: replace with Hikai internal implementations
#include <unordered_map>

namespace hk {

// INFO: Codes from 0 to 100 reserved by Hikai
enum EventCode : u32;

// INFO: Codes from 0 to 100 reserved by Hikai
enum ErrorCode : u64;
HKAPI const char* getErrocodeStr(hk::ErrorCode error);

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

}

class EventSystem {
public:
    // TODO: change userdata to const &
    // using EventCallback = void(*)(EventContext userdata);
    using EventCallback = std::function<void(hk::EventContext userdata)>;

    struct Event {
        void *sender;
        u32 code;
        hk::EventContext userdata;
    };

    struct Subscriber {
        void *listener;
        EventCallback callback;
    };

public:
    HKAPI static EventSystem *instance();

    HKAPI b8 subscribe(u32 code, EventCallback callback,
                       void *listener = nullptr);

    HKAPI b8 unsubscribe(u32 code, EventCallback callback,
                         void *listener = nullptr);

    HKAPI b8 fireEvent(u32 code, const hk::EventContext &userdata,
                       void *sender = nullptr);

    void init();
    void deinit();
    void dispatch();

    EventSystem(EventSystem &other) = delete;
    void operator=(const EventSystem&) = delete;

protected:
    EventSystem() {}
    static EventSystem *singleton;

private:
    std::unordered_map<u32, hk::vector<Subscriber>> subscribers;
    // TODO: make size customizable, maybe through init
    hk::ring_buffer<Event, 100, true> buffer;

};

enum hk::EventCode : u32 {
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

    // i16[0] = delta
    EVENT_MOUSE_WHEEL,

    // u32[0] = width, u32[1] = height
    EVENT_WINDOW_RESIZE,

    MAX_EVENT_CODES
};

enum hk::ErrorCode : u64 {
    ERROR_UNKNOWN = 0,

    // TODO: add error codes as needed

    ERROR_UNSUPPORTED_GRAPHICS_API,

    MAX_ERROR_CODES
};

#endif // HK_EVENTSYSTEM_H
