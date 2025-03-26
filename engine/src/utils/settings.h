#ifndef HK_SETTINGS_H
#define HK_SETTINGS_H

#include "strings/hkstring.h"
#include "utility/hktypes.h"
#include "math/vec2f.h"

#include <variant>
#include <unordered_set>

namespace hk {

enum class Setting {
    EMPTY = 0,

    WINDOW_SIZE,

    FRAMES_IN_FLIGHT,

    // setting: basic on or off
    // constraints: basic supported or not
    BINDLESS_RESOURCES,

    /* Vulkan specific */

    // setting: string_set of layers
    // constraints: string_set of supported layers
    VK_LAYERS,
    // NOTE: can be called before instance creation
    VK_INST_EXTENSIONS,
    VK_DEVICE_EXTENSIONS,

    MAX_SETTINGS
};

using string_set = std::unordered_set<const char*>;
using SettingVariant = std::variant<
    u32,        // basic
    hkm::vec2f, // range
    string_set  // strings
>;

HKAPI void set_setting(Setting setting, const SettingVariant &value);

HKAPI SettingVariant get_setting(Setting setting);
HKAPI SettingVariant get_constraints(Setting setting);

void set_constraints(Setting setting, const SettingVariant &constraints);

#if defined(HKDEBUG)
// SettingData get_settings_data();
#endif

}

#endif // HK_SETTINGS_H
