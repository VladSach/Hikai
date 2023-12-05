#ifndef HK_TIMER_H
#define HK_TIMER_H

#include "defines.h"

#include <chrono>

namespace hk {

// TODO: change to platform dependent implementations
class Timer {
private:
    using highres_clock = std::chrono::high_resolution_clock;
    std::chrono::time_point<highres_clock> timestamp;

public:
    HKAPI inline void record()
    {
        timestamp = std::chrono::high_resolution_clock::now();
    }

    HKAPI inline f64 elapsed()
    {
        auto result = std::chrono::duration_cast<std::chrono::seconds>(
                        highres_clock::now() - timestamp).count();
        return static_cast<f64>(result);
    }

    HKAPI inline f64 elapsed(std::chrono::time_point<highres_clock> time)
    {
        auto result = std::chrono::duration_cast<std::chrono::seconds>(
                        time - timestamp).count();
        return static_cast<f64>(result);
    }

    HKAPI inline f64 update()
    {
        auto now = highres_clock::now();
        f64 saved = elapsed(now);
        timestamp = now;
        return saved;
    }
};

};
#endif // HK_TIMER_H
