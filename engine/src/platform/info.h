#ifndef HK_INFO_H
#define HK_INFO_H

#include "defines.h"
#include "platform.h"
#include "utils/containers/hkvector.h"

namespace hk::platform {

struct MonitorInfo {
    std::string name;

    // Resolution
    u32 width = 0;
    u32 height = 0;

    f32 scale = 1.f;

    // Refresh Rate
    u32 hz = 0;

    // Color Depth
    u32 depth = 0;
};

HKAPI void updateMonitorInfo();
HKAPI MonitorInfo getMonitorInfo();
HKAPI hk::vector<MonitorInfo> &getAllMonitorInfos();

// Should be fine
#pragma warning(disable : 4201)
struct CpuInfo {
    std::string brand;

    u32 cores;
    u32 threads; // Logical Processors
    u32 page_size;

    struct Cache {
        struct CacheInfo {
            u32 size; // in bytes

            u32 count;

            u32 sets;
            u32 associativity; // Ways of cache associativity

            u32 max_threads; // Number logical processors sharing this cache
            u32 max_cores;   // Number of processor cores in physical package

            u32 line_size;   // System coherency line size in bytes
            u32 partitions;  // Physical Line Partitions
            b8 is_self_initializing;
            b8 is_fully_associative;
        };

        struct L1 {
            CacheInfo data;
            CacheInfo instr;
        } L1;
        CacheInfo L2;
        CacheInfo L3;
    } cache;

    union SIMD {
        struct {
            u8 mmx    : 1;

            u8 sse    : 1;
            u8 sse2   : 1;
            u8 sse3   : 1;
            u8 ssse3  : 1;
            u8 sse4_1 : 1;
            u8 sse4_2 : 1;
            u8 sse4a  : 1;

            u8 fma3   : 1;

            u8 avx    : 1;
            u8 avx2   : 1;
            u8 avx512 : 1; // only foundation
        };
        u32 flags;
    } simd;

    union Features {
        struct {
            u8 vme        : 1; // Virtual 8086 mode extensions
            u8 pse        : 1; // Page Size Extension (4 MB pages)
            u8 pae        : 1; // Physical Address Extension
            u8 mce        : 1; // Machine Check Exception
            u8 mca        : 1; // Machine check architecture
            u8 vmx        : 1; // Virtual Machine eXtensions
            u8 pcid       : 1; // Process context identifiers (CR4 bit 17)
            u8 popcnt     : 1; // POPCNT instruction
            u8 aes        : 1; // AES instruction set
            u8 hypervisor : 1;

            u8 sha        : 1;
            u8 sha512     : 1;

            u8 tce        : 1; // Translation Cache Extension
            u8 tbm        : 1; // Trailing Bit Manipulation
            u8 lm         : 1; // Long mode
        };
        u64 features;
    } feature;

    u32 numa_nodes;
    u32 physical_packages;

    std::string vendor;

    struct VersionInfo {
        u32 stepping = 0;
        u32 model = 0;
        u32 family = 0;
        u32 processor_type = 0; // Only Intel
    } version;

    u32 CLFLUSH_size = 0; // in bytes
    u32 APIC_id = 0;

    u32 physical_address = 0; // Number of Physical Address Bits
    u32 virtual_address = 0;  // Number of Linear Address Bits
    u32 physical_threads = 0;
};
#pragma warning(default : 4201)

HKAPI void updateCpuInfo();
HKAPI CpuInfo getCpuInfo();

}

#endif // HK_INFO_H
