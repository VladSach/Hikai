#ifndef HK_WAYLAND_UTILS_H
#define HK_WAYLAND_UTILS_H

#include "wltypes.h"
#include "utility/hkassert.h"

#include <utility>

namespace hk::platform::wl {

//The maximum size of a protocol message
constexpr u32 max_message_size = 4096;

// #define roundup_4(n) (((n) + 3) & -4)
constexpr u32 roundup_4(u32 n) { return ((n) + 3) & -4; }

template<u32 N>
class wire_message {
public:
    wire_message() {}
    wire_message(u8 *msg, u64 size)
    {
        copy(msg, size);
    }

    constexpr void copy(u8 *msg, u64 size)
    {
        // Maybe there's a better choice, but I don't want to rewrite this
        // backend for the n-th time, so yeah, memcpy
        memcpy(buffer_, msg, size);
        size_ = size;
    }

    constexpr void write_u32(u32 data)
    {
        ALWAYS_ASSERT(size_ + sizeof(data) <= N);
        ALWAYS_ASSERT(((u64)buffer_ + size_) % sizeof(data) == 0);

        *(reinterpret_cast<u32*>(buffer_ + size_)) = data;
        size_ += sizeof(data);
    }

    constexpr void write_u16(u16 data)
    {
        ALWAYS_ASSERT(size_ + sizeof(data) <= N);
        ALWAYS_ASSERT(((u64)buffer_ + size_) % sizeof(data) == 0);

        *(reinterpret_cast<u16*>(buffer_ + size_)) = data;
        size_ += sizeof(data);
    }

    constexpr void write_string(const char *src, u32 length)
    {
        ALWAYS_ASSERT(size_ + length <= N);

        write_u32(length);
        memcpy(buffer_ + size_, src, roundup_4(length));
        size_ += roundup_4(length);
    }

    constexpr u32 read_u32()
    {
        ALWAYS_ASSERT(size_ >= sizeof(u32));
        ALWAYS_ASSERT((size_t)buffer_ % sizeof(u32) == 0);

        u32 out = *(u32 *)(buffer_ + index);
        index += sizeof(out);
        size_ -= sizeof(out);

        return out;
    }

    constexpr u16 read_u16() {
        ALWAYS_ASSERT(size_ >= sizeof(u16));
        ALWAYS_ASSERT((size_t)buffer_ % sizeof(u16) == 0);

        u16 out = *(u16 *)(buffer_ + index);
        index += sizeof(out);
        size_ -= sizeof(out);

        return out;
    }

    // Could be a string or an array
    constexpr void read_blob(char *dst, u64 n) {
        ALWAYS_ASSERT(size_ >= n);

        memcpy(dst, buffer_ + index, n);

        index += n;
        size_ -= n;
    }

    constexpr u64 size() const { return size_; }
    constexpr u8* buffer() { return buffer_; }
    constexpr const u8* buffer() const { return buffer_; }

private:
    u64 size_ = 0;
    u8 buffer_[N] = {};

    u32 index = 0;
};



// TODO: maybe move to utils/to_stings
constexpr const char* to_string(Subpixel subpixel)
{
    constexpr const char* lookup_type[] = {
        "Unknown Geometry",
        "No Geometry",
        "Horizontal RGB",
        "Horizontal BGR",
        "Vertical RGB",
        "Vertical BGR",
    };

    return lookup_type[static_cast<u32>(subpixel)];
}

constexpr const char* to_string(Format format)
{
    constexpr std::pair<Format, char const *> lookup_map[] =
    {
        { Format::argb8888, "ARGB8888" },
        { Format::xrgb8888, "XRGB8888" },

        { Format::c8,     "C8" },
        { Format::rgb332, "RGB332" },
        { Format::bgr233, "BGR233" },

        { Format::xrgb4444, "XRGB4444" },
        { Format::xbgr4444, "XBGR4444" },
        { Format::rgbx4444, "RGBX4444" },
        { Format::bgrx4444, "BGRX4444" },
        { Format::argb4444, "ARGB4444" },
        { Format::abgr4444, "ABGR4444" },
        { Format::rgba4444, "RGBA4444" },
        { Format::bgra4444, "BGRA4444" },

        { Format::xrgb1555, "XRGB1555" },
        { Format::xbgr1555, "XBGR1555" },
        { Format::rgbx5551, "RGBX5551" },
        { Format::bgrx5551, "BGRX5551" },
        { Format::argb1555, "ARGB1555" },
        { Format::abgr1555, "ABGR1555" },
        { Format::rgba5551, "RGBA5551" },
        { Format::bgra5551, "BGRA5551" },

        { Format::rgb565, "RGB565" },
        { Format::bgr565, "BGR565" },

        { Format::rgb888, "RGB888" },
        { Format::bgr888, "BGR888" },

        { Format::xbgr8888, "XBGR8888" },
        { Format::rgbx8888, "RGBX8888" },
        { Format::bgrx8888, "BGRX8888" },
        { Format::abgr8888, "ABGR8888" },
        { Format::rgba8888, "RGBA8888" },
        { Format::bgra8888, "BGRA8888" },

        { Format::xrgb2101010, "XRGB2101010" },
        { Format::xbgr2101010, "XBGR2101010" },
        { Format::rgbx1010102, "RGBX1010102" },
        { Format::bgrx1010102, "BGRX1010102" },
        { Format::argb2101010, "ARGB2101010" },
        { Format::abgr2101010, "ABGR2101010" },
        { Format::rgba1010102, "RGBA1010102" },
        { Format::bgra1010102, "BGRA1010102" },

        { Format::yuyv, "YUYV" },
        { Format::yvyu, "YVYU" },
        { Format::uyvy, "UYVY" },
        { Format::vyuy, "VYUY" },
        { Format::ayuv, "AYUV" },

        { Format::nv12, "NV12" },
        { Format::nv21, "NV21" },
        { Format::nv16, "NV16" },
        { Format::nv61, "NV61" },

        { Format::yuv410, "YUV410" },
        { Format::yvu410, "YVU410" },
        { Format::yuv411, "YUV411" },
        { Format::yvu411, "YVU411" },
        { Format::yuv420, "YUV420" },
        { Format::yvu420, "YVU420" },
        { Format::yuv422, "YUV422" },
        { Format::yvu422, "YVU422" },
        { Format::yuv444, "YUV444" },
        { Format::yvu444, "YVU444" },

        { Format::r8,            "R8"            },
        { Format::r16,           "R16"           },
        { Format::rg88,          "RG88"          },
        { Format::gr88,          "GR88"          },
        { Format::rg1616,        "RG1616"        },
        { Format::gr1616,        "GR1616"        },
        { Format::xrgb16161616f, "XRGB16161616f" },
        { Format::xbgr16161616f, "XBGR16161616f" },
        { Format::argb16161616f, "ARGB16161616f" },
        { Format::abgr16161616f, "ABGR16161616f" },

        { Format::xyuv8888,  "XYUV8888"  },
        { Format::vuy888,    "VUY888"    },
        { Format::vuy101010, "VUY101010" },

        { Format::y210, "Y210" },
        { Format::y212, "Y212" },
        { Format::y216, "Y216" },
        { Format::y410, "Y410" },
        { Format::y412, "Y412" },
        { Format::y416, "Y416" },

        { Format::xvyu2101010,     "XVYU2101010"     },
        { Format::xvyu12_16161616, "XVYU12_16161616" },
        { Format::xvyu16161616,    "XVYU16161616"    },

        { Format::y0l0, "Y0l0" },
        { Format::x0l0, "X0l0" },
        { Format::y0l2, "Y0l2" },
        { Format::x0l2, "X0l2" },

        { Format::yuv420_8bit,  "YUV420 8bit," },
        { Format::yuv420_10bit, "YUV420 10bit" },

        { Format::xrgb8888_a8, "XRGB8888_A8" },
        { Format::xbgr8888_a8, "XBGR8888_A8" },
        { Format::rgbx8888_a8, "RGBX8888_A8" },
        { Format::bgrx8888_a8, "BGRX8888_A8" },

        { Format::rgb888_a8, "RGB888_A8" },
        { Format::bgr888_a8, "BGR888_A8" },
        { Format::rgb565_a8, "RGB565_A8" },
        { Format::bgr565_a8, "BGR565_A8" },

        { Format::nv24, "NV24" },
        { Format::nv42, "NV42" },
        { Format::p210, "P210" },
        { Format::p010, "P010" },
        { Format::p012, "P012" },
        { Format::p016, "P016" },

        { Format::axbxgxrx106106106106, "AXBXGXRX106106106106" },

        { Format::nv15, "NV15" },

        { Format::q410, "Q410" },
        { Format::q401, "Q401" },

        { Format::xrgb16161616, "XRGB16161616" },
        { Format::xbgr16161616, "XBGR16161616" },
        { Format::argb16161616, "ARGB16161616" },
        { Format::abgr16161616, "ABGR16161616" },

        { Format::c1, "C1" },
        { Format::c2, "C2" },
        { Format::c4, "C4" },
        { Format::d1, "D1" },
        { Format::d2, "D2" },
        { Format::d4, "D4" },
        { Format::d8, "D8" },
        { Format::r1, "R1" },
        { Format::r2, "R2" },
        { Format::r4, "R4" },

        { Format::r10, "R10" },
        { Format::r12, "R12" },

        { Format::avuy8888, "AVUY8888" },
        { Format::xvuy8888, "XVUY8888" },

        { Format::p030, "P030" },
    };

    constexpr u64 range = sizeof(lookup_map) / sizeof(lookup_map[0]);
    for (u32 i = 0; i < range; ++i) {
        if (lookup_map[i].first == format) {
            return lookup_map[i].second;
        }
    }

    return "Undefined";
}

}

#endif // HK_WAYLAND_UTILS_H
