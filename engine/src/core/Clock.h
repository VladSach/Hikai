#ifndef HK_CLOCK_H
#define HK_CLOCK_H

#include "defines.h"

#include <chrono>

namespace hk {

class Clock {
private:
    using highres_clock = std::chrono::high_resolution_clock;
    std::chrono::time_point<highres_clock> timestamp;

public:
    inline void record()
    {
        timestamp = std::chrono::high_resolution_clock::now();
    }

    inline f64 elapsed()
    {
        auto result =
            std::chrono::duration_cast<std::chrono::duration<double>>(
                highres_clock::now() - timestamp).count();
        return static_cast<f64>(result);
    }

    inline f64 elapsed(std::chrono::time_point<highres_clock> time)
    {
        auto result =
            std::chrono::duration_cast<std::chrono::duration<double>>(
                time - timestamp).count();
        return static_cast<f64>(result);
    }

    inline f64 update()
    {
        auto now = highres_clock::now();
        f64 saved = elapsed(now);
        timestamp = now;
        return saved;
    }
};

};
#endif // HK_CLOCK_H
