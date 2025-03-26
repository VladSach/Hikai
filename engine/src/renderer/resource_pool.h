#ifndef HK_RESOURCE_POOL_H
#define HK_RESOURCE_POOL_H

#include "renderer/resource_types.h"

#include <deque>

namespace hk::bkr {

using ResourceHandle = Handle<struct ResourceTag>;

template <typename ResourceType, typename DescType>
class resource_pool {
public:
void init()
{
    constexpr u32 initial_size = 2048;
    pool.resize(initial_size);
    metas.resize(initial_size);
    descs.resize(initial_size);

    free_indices.resize(initial_size);
    for (u32 i = initial_size - 1; i > 0; --i) {
        free_indices.push_back(i);
    }
    free_indices.push_back(0);

    count = 0;
}

void create_resource(const ResourceType &resource,
                     const DescType &desc,
                     const hk::string &name)
{
    u32 idx = free_indices.back();
    free_indices.pop_back();

    auto &slot = pool.at(idx);
    slot.data = resource;
    slot.is_valid = true;

    BufferHandle handle = { idx, slot.gen };

    ResourceMetadata meta;
    meta.name = name;
    meta.handle.value = handle.value;

    metas.at(idx) = meta;
    descs.at(idx) = desc;

    ++count;
}

void destroy_resource(const ResourceHandle &handle)
{
    // TODO: delete desc, meta, mark index free, add checks, etc

    Slot &slot = pool.at(handle.index);
    ALWAYS_ASSERT(handle.gen == slot.gen);
    ALWAYS_ASSERT(slot.is_valid);

    // TODO: push to dealloc queue
    deallocate_buffer(slot.data);

    ++slot.gen;
    slot.is_valid = false;

    free_indices.push_back(handle.index);

    --count;
}

private:
    struct Slot {
        ResourceType data;
        u32 gen = 0;
        b8 is_valid = false;
    };

    u32 count;

    // TODO: change to more suitable structure, maybe colony, maybe pool
    hk::vector<Slot> pool;
    // TODO: change to hk::stack
    std::deque<u32> free_indices;

    hk::vector<ResourceMetadata> metas;

    hk::vector<DescType> descs;
};

}

#endif // HK_RESOURCE_POOL_H
