#ifndef HKRING_BUFFER_H
#define HKRING_BUFFER_H

#include "defines.h"
#include "utils/Logger.h"

// TODO: either use atomic or mutex
namespace hk {

template<typename T, u32 N, b8 overwrite = false>
class ring_buffer {
private:
    u32 tail;
    u32 head;

    u32 size;
    T buffer[N];
public:
    ring_buffer() : tail(0), head(0), size(0) {}
    ~ring_buffer() { clear(); }

    constexpr T& operator[](u32 index) noexcept
	{
		ALWAYS_ASSERT((size > 0 && index < size), "Out of bounds");
		return buffer[index];
	}

	constexpr const T& operator[](size_t index) const noexcept
	{
		ALWAYS_ASSERT((size > 0 && index < size), "Out of bounds");
		return buffer[index];
	}

    bool push(const T &value)
    {
        if (size == N) {
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

        ++size;

        return true;
    }

    bool pop(T &value)
    {
        if (!size) {
            LOG_WARN("Trying to read value from empty ring_buffer");
            return false;
        }

        --size;

        value = buffer[head];
        head = (head + 1) % N;
        return true;
    }

    bool peek(T &value) const
    {
        if (!size) {
            LOG_WARN("Trying to read value from empty ring_buffer");
            return false;
        }

        value = buffer[head];
        return true;
    }

    constexpr void clear() noexcept
    {
        while(size--) {
            T value; pop(value);
            value.~T();
        }
        tail = 0; head = 0; size = 0;
    }

    inline u32 getSize() const { return size; }
};

}

#endif // HKRING_BUFFER_H
