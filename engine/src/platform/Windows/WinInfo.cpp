#include "platform/info.h"

#include <intrin.h>

namespace hk::platform {

static hk::vector<MonitorInfo> infos;
static CpuInfo cpu_info;

HKAPI BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor,
                                    HDC hdcMonitor,
                                    LPRECT lprcMonitor, LPARAM dwData);

void updateMonitorInfo()
{
    infos.clear();

    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);

    DISPLAY_DEVICE dd;
    dd.cb = sizeof(dd);
    u32 deviceIndex = 0;
    while (EnumDisplayDevices(0, deviceIndex, &dd, 0)) {
        u32 monitorIndex = 0;
        while (EnumDisplayDevices(dd.DeviceName, monitorIndex, &dd, 0)) {
            infos[monitorIndex].name = dd.DeviceName;
            infos[monitorIndex].name += ", ";
            infos[monitorIndex].name += dd.DeviceString;
            ++monitorIndex;
        }
        ++deviceIndex;
    }
}

MonitorInfo getMonitorInfo()
{
    // FIX: determine which monitor is main
    return infos.at(0);
}

hk::vector<MonitorInfo> &getAllMonitorInfos()
{
    return infos;
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                              LPRECT lprcMonitor, LPARAM dwData)
{
    (void)hdcMonitor, (void)lprcMonitor, (void)dwData;

    hk::platform::MonitorInfo out;

    /* Source:
     * https://stackoverflow.com/questions/70976583/
     * get-real-screen-resolution-using-win32-api
     */

    MONITORINFOEX info = { sizeof(MONITORINFOEX) };
    GetMonitorInfo(hMonitor, &info);

    DEVMODE devmode = {};
    devmode.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &devmode);

    // It returns native resolution in any case,
    // even if the OS tries to lie due to the DPI awareness of the process
    out.width = devmode.dmPelsWidth;
    out.height = devmode.dmPelsHeight;

    // Always >= 96
    u32 win32dpi = GetDpiForSystem();

    // Values can be negative if not the primary monitor
    i32 virtual_width = info.rcMonitor.right - info.rcMonitor.left;

    f32 virtual_to_real_ratio = virtual_width / static_cast<f32>(out.width);

    out.scale = win32dpi / 96.f / virtual_to_real_ratio;

    out.hz = devmode.dmDisplayFrequency;
    out.depth = devmode.dmBitsPerPel;

    infos.push_back(out);
    return TRUE;
}

// TODO: Return to cpu info one day
#pragma warning(disable : 4201)
struct CPUID {
    union {
        u32 registers[4] = { 0 };

        struct {
            u32 eax;
            u32 ebx;
            u32 ecx;
            u32 edx;
        };
    };

    void call(u32 EAX, u32 ECX = 0)
    {
#ifdef _MSC_VER
    // Year 2025, MSVC still doesn't support inline asm under x86_64
    __cpuidex(reinterpret_cast<i32*>(registers),
              static_cast<i32>(EAX),
              static_cast<i32>(ECX));
#else
    asm ("cpuid"
        :
            "=a" (registers[0]),
            "=b" (registers[1]),
            "=c" (registers[2]),
            "=d" (registers[3])
        : "a" (EAX), "c" (ECX));
#endif
    }
};
#pragma warning(default : 4201)

void updateCpuInfo()
{
    /* ==== WinAPI ==== */

    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    cpu_info.page_size = sysinfo.dwPageSize;

    // https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getlogicalprocessorinformation

    DWORD return_length = 0;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer = NULL;

    // Get required buffer size
    while (true) {
        DWORD rc = GetLogicalProcessorInformation(buffer, &return_length);

        if (rc != FALSE) { break; }

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
            ALWAYS_ASSERT(false, "Error:", GetLastError());
        }

        if (buffer) { free(buffer); }

        buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(return_length);

        ALWAYS_ASSERT(buffer, "Failed to allocate processor info buffer");
    }

    u64 byte_offset = 0;
    PCACHE_DESCRIPTOR Cache;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = buffer;
    u64 info_size = sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);

    // Used after CPUID section
    DWORD processorL1CacheCount = 0;
    DWORD processorL2CacheCount = 0;
    DWORD processorL3CacheCount = 0;

    while (byte_offset + info_size <= return_length) {
        switch (ptr->Relationship) {
        case RelationNumaNode: {
            // Non-NUMA systems report a single record of this type
            cpu_info.numa_nodes++;
        } break;

        case RelationProcessorCore: {
            cpu_info.cores++;
        } break;

        case RelationCache: {
            // Cache data is in ptr->Cache,
            // one CACHE_DESCRIPTOR structure for each cache
            Cache = &ptr->Cache;

            if      (Cache->Level == 1) { processorL1CacheCount++; }
            else if (Cache->Level == 2) { processorL2CacheCount++; }
            else if (Cache->Level == 3) { processorL3CacheCount++; }

        } break;

        case RelationProcessorPackage: {
            // Logical processors share a physical package
            cpu_info.physical_packages++;
        } break;

        default:
            LOG_ERROR("Unsupported LOGICAL_PROCESSOR_RELATIONSHIP value");
        }

        byte_offset += info_size;
        ptr++;
    }

    free(buffer);

    /* ==== CPUID ==== */

    // https://en.wikipedia.org/wiki/CPUID
    // https://www.amd.com/content/dam/amd/en/documents/processor-tech-docs/programmer-references/24594.pdf

    CPUID cpuid;

    // TODO: add checks
    u32 max_leaf = 0; // Highest Function Parameter
    u32 ext_max_leaf = 0; // Highest Extended Function Parameter
    b8 is_intel = true; // If false = AMD

    /* ====== EAX=0: Highest Function Parameter and Manufacturer ID ====== */

    /* This returns 2 things:
     * 1. The largest value EAX can be set to before calling CPUID
     * is returned in EAX.
     * 2. The CPU's manufacturer ID string – a twelve-character ASCII string
     * stored in EBX, EDX, ECX (in that order). */

    cpuid.call(0);

    max_leaf = cpuid.eax;

    std::string vendor;
    vendor += std::string(reinterpret_cast<const char*>(&cpuid.ebx), 4);
    vendor += std::string(reinterpret_cast<const char*>(&cpuid.edx), 4);
    vendor += std::string(reinterpret_cast<const char*>(&cpuid.ecx), 4);

    cpu_info.vendor = vendor;

    if (cpu_info.vendor == "GenuineIntel") {
        is_intel = true;
    } else if (cpu_info.vendor != "AuthenticAMD") {
        ALWAYS_ASSERT(0, "Only Intel and AMD CPUs are supported");
    }

    /* ====== EAX=1: Processor Info and Feature Bits ====== */

    /* This returns the CPU's stepping, model, and family information
     * in register EAX (also called the signature of a CPU),
     * feature flags in registers EDX and ECX,
     * and additional feature info in register EBX. */

    cpuid.call(1);

    // Processor Info
    cpu_info.version.stepping       = (cpuid.eax >> 0)  & 0xf;
    cpu_info.version.model          = (cpuid.eax >> 4)  & 0xf;
    cpu_info.version.family         = (cpuid.eax >> 8)  & 0xf;
    cpu_info.version.processor_type = (cpuid.eax >> 12) & 0x3;
    u32 extended_model              = (cpuid.eax >> 16) & 0xf;
    u32 extended_family             = (cpuid.eax >> 20) & 0xff;

    if (cpu_info.version.family == 6 || cpu_info.version.family == 15) {
        cpu_info.version.model += (extended_model << 4);
    }

    if (cpu_info.version.family == 15) {
        cpu_info.version.family += extended_family;
    }

    // https://www.os2museum.com/wp/htt-means-hyper-threading-right/
    b8 cflush = cpuid.edx >> 14 & 0x01;
    b8 htt    = cpuid.edx >> 28 & 0x01;

    // u32 brand_id          =          (cpuid.ebx  >> 0)  & 0xff;
    cpu_info.CLFLUSH_size = cflush ? ((cpuid.ebx >> 8)  & 0xff) * 8 : 0;
    cpu_info.threads      = htt    ? (cpuid.ebx  >> 16) & 0xff : 0;
    cpu_info.APIC_id      =          (cpuid.ebx  >> 24) & 0xff;

    // Features Bits
    cpu_info.simd.sse3          = cpuid.ecx >> 0  & 0x01;
    cpu_info.feature.vmx        = cpuid.ecx >> 5  & 0x01;
    cpu_info.simd.ssse3         = cpuid.ecx >> 9  & 0x01;
    cpu_info.simd.fma3          = cpuid.ecx >> 12 & 0x01;
    cpu_info.feature.pcid       = cpuid.ecx >> 17 & 0x01;
    cpu_info.simd.sse4_1        = cpuid.ecx >> 19 & 0x01;
    cpu_info.simd.sse4_2        = cpuid.ecx >> 20 & 0x01;
    cpu_info.feature.popcnt     = cpuid.ecx >> 23 & 0x01;
    cpu_info.feature.aes        = cpuid.ecx >> 25 & 0x01;
    cpu_info.simd.avx           = cpuid.ecx >> 28 & 0x01;
    cpu_info.feature.hypervisor = cpuid.ecx >> 31 & 0x01;

    cpu_info.feature.vme = cpuid.edx >> 1  & 0x01;
    cpu_info.feature.pse = cpuid.edx >> 3  & 0x01;
    cpu_info.feature.pae = cpuid.edx >> 6  & 0x01;
    cpu_info.feature.mce = cpuid.edx >> 7  & 0x01;
    cpu_info.feature.mca = cpuid.edx >> 14 & 0x01;
    cpu_info.simd.mmx    = cpuid.edx >> 23 & 0x01;
    cpu_info.simd.sse    = cpuid.edx >> 25 & 0x01;
    cpu_info.simd.sse2   = cpuid.edx >> 26 & 0x01;

    /* ====== EAX=7, ECX=0: Extended Features ====== */

    cpuid.call(7, 0);

    cpu_info.simd.avx2   = cpuid.ebx >> 5  & 0x01;
    cpu_info.simd.avx512 = cpuid.ebx >> 16 & 0x01;
    cpu_info.feature.sha = cpuid.ebx >> 29 & 0x01;

    /* ====== EAX=7, ECX=1: Extended Features ====== */

    cpuid.call(7, 1);

    cpu_info.feature.sha512 = cpuid.eax >> 0 & 0x01;

    /* ====== EAX=16h: CPU and Bus Clock Frequencies ====== */

    /* Works on Intel Skylake, Kaby Lake and newer processors only
     * No idea how to do this on AMD */

    // cpuid.call(0x16);
    //
    // u32 base_cpu_frequency = cpuid.eax & 0xffff;
    // u32 max_cpu_frequency  = cpuid.ebx & 0xffff;
    // u32 bus_frequency      = cpuid.ecx & 0xffff;

    /* ====== EAX=8000'0000h: Highest Extended Function Implemented ====== */

    /* 1. The largest value EAX can be set to before calling CPUID
     * is returned in EAX.
     * 2. EBX/ECX/EDX return the manufacturer ID string (same as EAX=0)
     * on AMD but not Intel CPUs. */

    cpuid.call(0x8000'0000);

    ext_max_leaf = cpuid.eax;

    /* ===== EAX=8000'0001h: Extended Processor Info and Feature Bits ===== */

    cpuid.call(0x8000'0001);

    cpu_info.simd.sse4a  = cpuid.ecx >> 6  & 0x01;
    cpu_info.feature.tce = cpuid.ecx >> 17 & 0x01;
    cpu_info.feature.tbm = cpuid.ecx >> 21 & 0x01;

    cpu_info.feature.lm  = cpuid.edx >> 29 & 0x01;

    /* === EAX=8000'0002h,8000'0003h,8000'0004h: Processor Brand String === */

    /* These return the processor brand string in EAX, EBX, ECX and EDX.
     * CPUID must be issued with each parameter in sequence to get
     * the entire 48-byte ASCII processor brand string.
     * It is necessary to check whether the feature is present in the CPU by
     * issuing CPUID with EAX = 80000000h first and checking if
     * the returned value is not less than 80000004h. */

    std::string brand;

    cpuid.call(0x8000'0002);
    brand += std::string(reinterpret_cast<const char*>(&cpuid.registers), 16);
    cpuid.call(0x8000'0003);
    brand += std::string(reinterpret_cast<const char*>(&cpuid.registers), 16);
    cpuid.call(0x8000'0004);
    brand += std::string(reinterpret_cast<const char*>(&cpuid.registers), 16);

    cpu_info.brand = brand;

    /* ====== EAX=8000'0008h: Virtual and Physical Address Sizes ====== */

    /* Feature bits in EBX, Size and Range fields in EAX, ECX, EDX */

    cpuid.call(0x8000'0008);

    cpu_info.physical_address = (cpuid.eax >> 0) & 0xff;
    cpu_info.virtual_address  = (cpuid.eax >> 8) & 0xff;

    cpu_info.physical_threads = ((cpuid.ecx >> 0) & 0xff) + 1;

    /* ====== EAX=8000'001Eh: Processor Topology Information ====== */

    /* Only AMD? */

    /* ====== Cache Hierarchy and Topology ====== */

    /* https://stackoverflow.com/a/64610070
     *
     * Intel CPUs:
     * for newer CPUs: use EAX=4 (with different values in ECX)
     * for older CPUs: use EAX=2 (use wiki)
     *      Wikipedia "EAX=2: Cache and TLB Descriptor Information"
     *
     * AMD CPUs:
     * for newer CPUs: use EAX=8000'001Dh (with different values in ECX)
     * for older CPUs: use EAX=8000'0005h (for L1 only), and
     *                 use EAX=8000'0006h (for L2 and L3 only)
     */


    /* === AMD === */
    /* --- NEW CPUs --- */

    struct CacheTopology {
        /* Type Values
         * Null
         * Data Cache
         * Instruction Cache
         * Unified Cache
         * Reserved */
        u32 type;

        u32 level;

        CpuInfo::Cache::CacheInfo info;
    };
    hk::vector<CacheTopology> caches;

    CacheTopology cache;

    for (u32 i = 0; ; ++i) {
        cpuid.call(0x8000'001D, i);

        if (!(cpuid.eax & 0xf0)) { break; }

        cache.type                       = (cpuid.eax >> 0) & 0x1f;
        cache.level                      = (cpuid.eax >> 5) & 0x03;
        cache.info.is_self_initializing  = (cpuid.eax >> 8) & 0x01;
        cache.info.is_fully_associative  = (cpuid.eax >> 9) & 0x01;

        cache.info.max_threads = ((cpuid.eax >> 14) & 0xfff) + 1;
        cache.info.max_cores   = is_intel ? ((cpuid.eax >> 26) & 0x3f) + 1 : 0;

        cache.info.line_size     = ((cpuid.ebx >> 0)  & 0xfff) + 1;
        cache.info.partitions    = ((cpuid.ebx >> 12) & 0x3ff) + 1;
        cache.info.associativity = ((cpuid.ebx >> 22) & 0x3ff) + 1;

        // For fully-associative caches, ECX should be treated as 0
        cache.info.sets = cache.info.is_fully_associative ? 0 : cpuid.ecx + 1;

        cache.info.size =
            cache.info.line_size * cache.info.partitions *
            cache.info.associativity * cache.info.sets;

        caches.push_back(cache);
    }

    for (auto c : caches) {
        if (c.level == 1) {
            if (c.type == 1) {
                cpu_info.cache.L1.data = c.info;
            } else {
                cpu_info.cache.L1.instr = c.info;
            }
        } else if (c.level == 2) {
            cpu_info.cache.L2 = c.info;
        } else if (c.level == 3) {
            cpu_info.cache.L3 = c.info;
        }
    }

    /* --- OLD CPUs --- */

    /* ====== EAX=8000'0005h: L1 Cache and TLB Identifiers ====== */

    /* This provides information about the processor's level-1 cache and TLB
     * EAX: information about L1 hugepage TLBs (TLBs that hold entries corresponding to 2M/4M pages)
     * EBX: information about L1 small-page TLBs (TLBs that hold entries corresponding to 4K pages)
     * ECX: information about L1 data cache
     * EDX: information about L1 instruction cache */

    // cpuid.call(0x8000'0005);

    /* ====== EAX=8000'0006h: Extended L2 Cache Features ====== */

    /* Returns details of the L2 cache in ECX, including
     * the line size in bytes (Bits 07 - 00),
     * type of associativity (encoded by a 4 bits field; Bits 15 - 12) and
     * the cache size in KB (Bits 31 - 16). */

    // cpuid.call(0x8000'0006);
    //
    // cpu_info.cache.L2.line_size     = (cpuid.ecx >> 0)  & 0xff;
    // cpu_info.cache.L2.associativity = (cpuid.ecx >> 12) & 0xf;
    // cpu_info.cache.L2.cache_size    = (cpuid.ecx >> 16) & 0xffff;

    cpu_info.cache.L1.data.count  = processorL1CacheCount / 2;
    cpu_info.cache.L1.instr.count = processorL1CacheCount / 2;
    cpu_info.cache.L2.count = processorL2CacheCount;
    cpu_info.cache.L3.count = processorL3CacheCount;
}

CpuInfo getCpuInfo()
{
    return cpu_info;
}

}

