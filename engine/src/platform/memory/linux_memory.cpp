#include "memory.h"

#include "platform/predef.h"

#ifdef HKLINUX

#include <sys/mman.h>

namespace hk {

void* memory_reserve(u64 size)
{
    return mmap(0, size, PROT_NONE, MAP_SHARED | MAP_ANONYMOUS, -1, (off_t)0);
}

b8 memory_commit(void *addr, u64 size)
{
    return !(mprotect(addr, size, PROT_READ | PROT_WRITE));
}

b8 memory_decommit(void *addr, u64 size)
{
    b8 result = false;
    result = !(mprotect(addr, size, PROT_NONE));
    result = !(madvise(addr, size, MADV_DONTNEED)) && result;
    return result;
}

b8 memory_release(void *addr, u64 size)
{
    return !(munmap(addr, size));
}

}

#endif // HKLINUX
