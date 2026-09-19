#ifndef HK_RING_BUFFER_H
#define HK_RING_BUFFER_H

#include "utility/hktypes.h"
#include "utility/hkassert.h"

namespace hk {

template<typename T, u32 N, b8 overwrite = false>
class ring_buffer {
public:
    ring_buffer() : tail_(0), head_(0), size_(0) {}
    ~ring_buffer() { clear(); }

    /* ===== Element access ===== */

    constexpr T& operator[](u32 index)
    {
        ALWAYS_ASSERT((size_ > 0 && index < size_), "Out of bounds");
        return buffer_[(head_ + index) % N];
    }

    constexpr const T& operator[](u32 index) const
    {
        ALWAYS_ASSERT((size_ > 0 && index < size_), "Out of bounds");
        return buffer_[(head_ + index) % N];
    }

    constexpr b8 peek(T &out) const
    {
        if (!size_) {
            // LOG_WARN("Trying to read value from empty ring_buffer");
            return false;
        }

        out = buffer_[head_];
        return true;
    }

    /* ===== Modifiers ===== */

    constexpr b8 push_back(const T &value)
    {
        if (size_ == N) {
            if (!overwrite) {
                // LOG_WARN("Trying to insert value in filled ring_buffer "
                //          "w/o overwrite option | Skipping push");
                return false;
            }

            buffer_[tail_] = value;
            tail_ = (tail_ + 1) % N;
            head_ = (head_ + 1) % N;

            return true;
        }

        buffer_[tail_] = value;
        tail_ = (tail_ + 1) % N;

        ++size_;

        return true;
    }

    constexpr b8 push_front(const T &value)
    {
        if (size_ == N) {
            if (!overwrite) {
                // LOG_WARN("Trying to insert value in filled ring_buffer "
                //          "w/o overwrite option | Skipping push");
                return false;
            }

            head_ = (head_ - 1 + N) % N;
            tail_ = (tail_ - 1 + N) % N;
            buffer_[head_] = value;

            return true;
        }

        head_ = (head_ + N - 1) % N;
        buffer_[head_] = value;

        ++size_;

        return true;
    }

    constexpr b8 pop_front(T &out)
    {
        if (!size_) {
            // LOG_WARN("Trying to read value from empty ring_buffer");
            return false;
        }

        out = buffer_[head_];
        head_ = (head_ + 1) % N;

        --size_;
        return true;
    }

    constexpr b8 pop_back(T &out)
    {
        if (!size_) {
            // LOG_WARN("Trying to read value from empty ring_buffer");
            return false;
        }

        tail_ = (tail_ + N - 1) % N;
        out = buffer_[tail_];

        --size_;
        return true;
    }

    constexpr b8 insert(u32 index, const T &value)
    {
        ALWAYS_ASSERT(index <= size_, "Out of bounds");

        if (size_ == N) {
            if (!overwrite) {
                // LOG_WARN("Trying to insert value in filled ring_buffer "
                //          "w/o overwrite option | Skipping insert");
                return false;
            }

            if (index <= size_ / 2) {
                for (u32 i = 0; i < index; ++i) {
                    u32 dst = (head_ + i - 1 + N) % N;
                    u32 src = (head_ + i)         % N;
                    buffer_[dst] = buffer_[src];
                }

                head_ = (head_ + N - 1) % N;
            } else {
                for (u32 i = size_; i > index; --i) {
                    u32 dst = (head_ + i)     % N;
                    u32 src = (head_ + i - 1) % N;
                    buffer_[dst] = buffer_[src];
                }
            }

            buffer_[(head_ + index) % N] = value;
            return true;
        }

        for (u32 i = size_; i > index; --i) {
            u32 dst = (head_ + i)     % N;
            u32 src = (head_ + i - 1) % N;
            buffer_[dst] = buffer_[src];
        }

        buffer_[(head_ + index) % N] = value;
        tail_ = (tail_ + 1) % N;

        ++size_;

        return true;
    }

    constexpr b8 erase(u32 index)
    {
        ALWAYS_ASSERT(index < size_, "Out of bounds");

        if (!size_) {
            // LOG_WARN("Trying to erase value from empty ring_buffer");
            return false;
        }

        if (index <= size_ / 2) {
            for (u32 i = index; i > 0; --i) {
                u32 dst = (head_ + i)     % N;
                u32 src = (head_ + i - 1) % N;
                buffer_[dst] = buffer_[src];
            }
            head_ = (head_ + 1) % N;
        } else {
            for (u32 i = index; i < size_ - 1; ++i) {
                u32 dst = (head_ + i)     % N;
                u32 src = (head_ + i + 1) % N;
                buffer_[dst] = buffer_[src];
            }
            tail_ = (tail_ + N - 1) % N;
        }

        --size_;

        return true;
    }

    // Erases all elements from the container
    constexpr void clear()
    {
        for (u32 i = 0; i < N; ++i) {
            buffer_[i].~T();
        }

        tail_ = 0; head_ = 0; size_ = 0;
    }

    /* ===== Capacity ===== */

    // Returns the number of elements
    constexpr u32 size() const { return size_; }

    /* Returns the number of elements that can be held
     * in currently allocated storage */
    constexpr u32 capacity() const { return N; }

    // Checks whether the container is empty
    constexpr b8 empty() const { return size_ == 0; }

private:
    // TODO: either use atomic or mutex

    u32 tail_;
    u32 head_;

    u32 size_;
    T buffer_[N];

};

} // hk

#endif // HK_RING_BUFFER_H
