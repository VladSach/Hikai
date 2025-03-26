#include "settings.h"

#include <array>

namespace hk {

struct SettingData {
    SettingVariant value;
    SettingVariant constraints;
};

static std::array<SettingData, static_cast<u32>(Setting::MAX_SETTINGS)> data;

void set_setting(Setting setting, const SettingVariant &value)
{
    u32 idx = static_cast<u32>(setting);

    switch (setting) {

    case Setting::WINDOW_SIZE:
        break;

    case Setting::FRAMES_IN_FLIGHT:
        break;

    case Setting::VK_LAYERS:
    case Setting::VK_INST_EXTENSIONS:
    case Setting::VK_DEVICE_EXTENSIONS: {
        string_set strings = std::get<string_set>(value);
        // TODO: add checks
        // TODO: check for incompetable combinations


    } break;

    default: { break; }
    }

    data.at(idx).value = value;
}

SettingVariant get_setting(Setting setting)
{
    return data.at(static_cast<u32>(setting)).value;
}

SettingVariant get_constraints(Setting setting)
{
    return data.at(static_cast<u32>(setting)).constraints;
}

void set_constraints(Setting setting, const SettingVariant &constraints)
{
    u32 idx = static_cast<u32>(setting);

    data.at(idx).constraints = constraints;
}

}
