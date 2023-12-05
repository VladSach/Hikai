#ifndef HK_RING_BUFFER_H
#define HK_RING_BUFFER_H

#include "defines.h"
#include "utils/Logger.h"

// TODO: either use atomic or mutex
namespace hk {

template<typename T, u32 N, b8 overwrite = false>
class ring_buffer {
private:
    u32 tail;
    u32 head;

    u32 size_;
    T buffer[N];

public:
    ring_buffer() : tail(0), head(0), size_(0) {}
    ~ring_buffer() { clear(); }

    constexpr T& operator[](u32 index) noexcept
    {
        ALWAYS_ASSERT((size_ > 0 && index < size_), "Out of bounds");
        return buffer[index];
    }

    constexpr const T& operator[](u32 index) const noexcept
    {
        ALWAYS_ASSERT((size_ > 0 && index < size_), "Out of bounds");
        return buffer[index];
    }

    inline bool push(const T &value)
    {
        if (size_ == N) {
            if (!overwrite) {
                LOG_WARN("Trying to insert value in filled ring_buffer "
                         "w/o overwrite option | Skipping push");
                return false;
            }

            buffer[tail] = value;
            tail = (tail + 1) % N;
            head = (head + 1) % N;

            return true;
        }

        buffer[tail] = value;
        tail = (tail + 1) % N;

        ++size_;

        return true;
    }

    inline bool pop(T &value)
    {
        if (!size_) {
            // LOG_WARN("Trying to read value from empty ring_buffer");
            return false;
        }

        --size_;

        value = buffer[head];
        head = (head + 1) % N;
        return true;
    }

    inline bool peek(T &value) const
    {
        if (!size_) {
            // LOG_WARN("Trying to read value from empty ring_buffer");
            return false;
        }

        value = buffer[head];
        return true;
    }

    constexpr void clear() noexcept
    {
        while(size_--) {
            T value; pop(value);
            value.~T();
        }
        tail = 0; head = 0; size_ = 0;
    }

    inline u32 size() const { return size_; }
};

}

#endif // HK_RING_BUFFER_H
