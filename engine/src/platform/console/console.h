#ifndef HK_CONSOLE
#define HK_CONSOLE

#include "hkcommon.h"
#include "utility/hktypes.h"

// TODO: change to Class and add support for multiple Consoles

namespace hk::platform {

HKAPI void alloc_console();
HKAPI void dealloc_console();

// Config Console
HKAPI b8 set_console_size(i16 cols, i16 rows);
HKAPI b8 set_console_title(const char *title);

// IO operations
HKAPI void write_console(const char *text);

}

#endif // HK_CONSOLE
