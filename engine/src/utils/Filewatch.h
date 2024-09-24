#ifndef HK_FILEWATCH_H
#define HK_FILEWATCH_H

#include "defines.h"

#include <string>
#include <functional>

namespace hk::filewatch {

enum class State {
    NONE = 0,

    /* WINAPI
    * FILE_ACTION_ADDED                   0x00000001
    * FILE_ACTION_REMOVED                 0x00000002
    * FILE_ACTION_MODIFIED                0x00000003
    * FILE_ACTION_RENAMED_OLD_NAME        0x00000004
    * FILE_ACTION_RENAMED_NEW_NAME        0x00000005 */

    ADDED,
    REMOVED,
    MODIFIED,
    RENAMED_OLD,
    RENAMED_NEW,

    MAX_FILE_STATE
};

using onStateChange = std::function<void(const std::string &path,
                                         const State state)>;

void init();
void deinit();
HKAPI void watch(const std::string &path, onStateChange callback);
HKAPI void unwatch(const std::string &path);

constexpr const char* stringFileState(State state)
{
    constexpr const char *lookup_file_state[] = {
        "NONE",

        "ADDED",
        "REMOVED",
        "MODIFIED",
        "RENAMED_OLD",
        "RENAMED_NEW",

        "MAX_FILE_STATE"
    };

    return lookup_file_state[static_cast<u32>(state)];
}

}

#endif // HK_FILEWATCH_H
