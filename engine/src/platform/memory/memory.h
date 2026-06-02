#ifndef HK_MEMORY_H
#define HK_MEMORY_H

#include "utility/hktypes.h"

namespace hk {

void* memory_reserve(u64 size);
b8 memory_commit(void *addr, u64 size);
b8 memory_decommit(void *addr, u64 size);
b8 memory_release(void *addr, u64 size);

}

#endif // HK_MEMORY_H
