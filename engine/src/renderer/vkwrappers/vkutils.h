#ifndef HK_VKUTILS_H
#define HK_VKUTILS_H

#include "vendor/vulkan/vulkan.h"

#include "hkstl/utility/hkassert.h"

#include <string>

namespace hk {

inline std::string vkApiToString(u32 version)
{
    std::string out;

    out += std::to_string(VK_API_VERSION_MAJOR(version));
    out += '.';
    out += std::to_string(VK_API_VERSION_MINOR(version));
    out += '.';
    out += std::to_string(VK_API_VERSION_PATCH(version));

    return out;
}

inline std::string vkDeviceTypeToString(VkPhysicalDeviceType type)
{
    constexpr const char* lookup_type[] = {
        "Other",
        "Integrated GPU",
        "Discrete GPU",
        "Virtual GPU",
        "CPU",
    };

    return lookup_type[static_cast<u32>(type)];
}

inline std::string vkDeviceDriveToString(u32 version, u32 vendorID)
{
    // Source:
    // https://github.com/SaschaWillems/vulkan.gpuinfo.org/blob/master/
    // includes/functions.php#L414

    std::string out;

    if (vendorID == 0x10DE /* NVIDIA */) {
        out += std::to_string((version >> 22) & 0x3ff);
        out += '.';
        out += std::to_string((version >> 14) & 0x0ff);
        out += '.';
        out += std::to_string((version >> 6) & 0x0ff);
        out += '.';
        out += std::to_string((version) & 0x003f);
    } else if (vendorID == 0x8086 /* INTEL */) {
        out += std::to_string(version >> 14);
        out += '.';
        out += std::to_string(version & 0x3fff);
    } else {
        // Use Vulkan version conventions if vendor mapping is not available
        out += std::to_string(VK_API_VERSION_MAJOR(version));
        out += '.';
        out += std::to_string(VK_API_VERSION_MINOR(version));
        out += '.';
        out += std::to_string(VK_API_VERSION_PATCH(version));
    }

    return out;
}

inline std::string vkDeviceVendorToString(u32 id)
{
    // Not gonna write all that
    // https://pcisig.com/membership/member-companies

    std::string out;

    switch (id) {
    case 0x1002: {
        out = "AMD";
    } break;

    case 0x10DE: {
        out = "NVIDIA";
    } break;

    case 0x13B5: {
        out = "ARM";
    } break;

    case 0x5143: {
        out = "Qualcomm";
    } break;

    case 0x8086: {
        out = "INTEL";
    } break;

    default:
        out = "Other";
    }

    return out;
}

}

#endif // HK_VKUTILS_H
