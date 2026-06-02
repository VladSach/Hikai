#include "memory.h"

#include "platform/predef.h"

#ifdef HKWIN32

#include <windows.h>

namespace hk {

void* memory_reserve(u64 size)
{
    return VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_READWRITE);
}

b8 memory_commit(void *addr, u64 size)
{
    void *out = VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE);
    return (out != 0);
}

b8 memory_decommit(void *addr, u64 size)
{
    return VirtualFree(addr, size, MEM_DECOMMIT);
}

b8 memory_release(void *addr, u64 size)
{
    return VirtualFree(addr, size, MEM_RELEASE);
}

}

#endif // HKWIN32
