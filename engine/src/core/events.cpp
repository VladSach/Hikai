#include "events.h"

#include "hkstl/Logger.h"

namespace hk::event {

static std::unordered_map<u32, std::vector<Subscriber>> subscribers;

// TODO: make size customizable, maybe through init
/* PERF: Don't want to add a separate event system for input
 * but without separating other events and input events
 * they just overflow buffer with mouse move events.
 * So current size is a compromise, I guess.
 * Probably will change it in the future, but right know it's not
 * an issue */
static hk::ring_buffer<Event, 1000, true> buffer;

void init()
{
    LOG_INFO("Event System initialized");
    buffer.clear();
    subscribers.clear();
}

void deinit()
{
    buffer.clear();
    subscribers.clear();
}

b8 subscribe(u32 code, const EventCallback &callback, void *listener)
{
    for (auto &sub : subscribers[code]) {
        if (sub.listener == listener &&
            sub.callback.target<EventCallback>() ==
            callback.target<EventCallback>())
        {
            LOG_WARN("Trying to register already existing callback");
            return false;
        }
    }

    subscribers[code].push_back({listener, callback});
    return true;
}

// TODO: maybe unsubscribe by uuid?
b8 unsubscribe(u32 code, const EventCallback &callback, void *listener)
{
    for (u32 i = 0; i < subscribers[code].size(); i++) {
        auto &sub = subscribers[code].at(i);
        if (sub.listener == listener &&
                sub.callback.target<EventCallback>() ==
                callback.target<EventCallback>())
        {
            subscribers[code].erase(subscribers[code].begin() + i);
            return true;
        }
    }

    LOG_WARN("Event to unsubscribe was not found");
    return false;

}

b8 fire(u32 code, const EventContext &userdata, void *sender)
{
    if (!subscribers[code].size()) { return true; }

    buffer.push({sender, code, userdata});
    return true;
}

void dispatch()
{
    Event event;
    while (buffer.pop(event)) {

        // LOG_DEBUG("Dispatching",
        //           getEventStr(static_cast<hk::EventCode>(event.code)),
        //           "to", subscribers[event.code].size(),
        //           "subscriber(s)");

        for (auto &sub : subscribers[event.code]) {
            sub.callback(event.userdata, sub.listener);
        }
    }
}

const char* getEventStr(EventCode code)
{
    constexpr char const *lookup_events[MAX_EVENT_CODES] = {
        "Empty Event",

        "App Shutdown Event",

        "Key Pressed Event",
        "Key Released Event",

        "Mouse Pressed Event",
        "Mouse Released Event",

        "Mouse Moved Event",

        "Raw Mouse Moved Event",

        "Mouse Wheel Event",

        "Window Resized Event",

        "Asset Loaded Event",
    };

    return lookup_events[code];
}

const char* getErrorCodeStr(ErrorCode error)
{
    constexpr char const *lookup_errors[MAX_ERROR_CODES] = {
        "Unknown error",

        "Unsupported Graphics API",
    };

    return lookup_errors[error];
}
}
