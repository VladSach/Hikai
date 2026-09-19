#include "platform/platform.h"
#ifdef HKWIN32

#include "clock.h"

namespace hk::pltf {

i32 get_wall_clock()
{
    SYSTEMTIME st, lt;

    GetSystemTime(&st); // UTC
    GetLocalTime(&lt);  // Local time

    return 0;
}

} // hk::pltf

#endif // HKWIN32
