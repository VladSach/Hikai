#ifndef HK_VECTOR_H
#define HK_VECTOR_H

#include "defines.h"

#include <initializer_list>

// TODO: replace with custom allocators
#include <memory>

namespace hk {

template<typename T>
class vector {
public:
    constexpr vector(u32 size = 0) { resize(size); }
    constexpr vector(u32 size, const T &value) { resize(size, value); }
    constexpr vector(const vector<T> &other) { *this = other; }
    constexpr vector(vector<T> &&other) { move(std::move(other)); }
    constexpr vector(std::initializer_list<T> list)
    {
        for (const auto &item : list) {
            emplace_back(item);
        }
    }

    inline ~vector()
    {
        clear();
        if (buffer_) { free(buffer_); }
    }

    constexpr vector<T>& operator=(const vector<T> &other)
    {
        clear();
        resize(other.size());
        for (u32 i = 0; i < size(); i ++) {
            buffer_[i] = other.buffer_[i];
        }
        return *this;
    }

    constexpr vector<T>& operator=(vector<T> &&other)
    {
        move(static_cast<vector<T>&&>(other));
        return *this;
    }

    constexpr T& operator[](u32 index)
    {
        ALWAYS_ASSERT((size_ > 0 && index < size_), "Out of bounds");
        return buffer_[index];
    }

    constexpr const T& operator[](u32 index) const
    {
        ALWAYS_ASSERT((size_ > 0 && index < size_), "Out of bounds");
        return buffer_[index];
    }

    constexpr void reserve(u32 capacity)
    {
        if (capacity <= capacity_) { return; }

        void *tmp = realloc(buffer_, capacity * sizeof(T));
        if (tmp) {
            buffer_ = static_cast<T*>(tmp);
            capacity_ = capacity;
        }
    }

    constexpr void resize(u32 size)
    {
        if (size > size_) {
            reserve(size);
            while (size_ < size) {
                new (buffer_ + size_++) T();
            }
        } else if (size < size_) {
            while (size_ > size) {
                pop_back();
            }
        }
    }

    constexpr void resize(u32 size, const T &value)
    {
        if (size > size_) {
            reserve(size);
            while (size_ < size) {
                emplace_back(value);
            }
        } else if (size < size_) {
            while (size_ > size) {
                pop_back();
            }
        }
    }

    constexpr void clear()
    {
        for (u32 i = 0; i < size_; i++) {
            buffer_[i].~T();
        }
        size_ = 0;
    }

    template <typename... Args>
    constexpr T& emplace_back(Args&& ...args)
    {
        if (size_ == capacity_) {
            reserve((capacity_ + 1) * 2);
        }

        new (buffer_ + size_) T(std::forward<Args>(args)...);
        ++size_;

        return buffer_[size_ - 1];
    }

    constexpr void push_back(const T& value)
    {
        emplace_back(value);
    }

    constexpr void push_back(T&& value)
    {
        emplace_back(static_cast<T&&>(value));
    }

    constexpr void pop_back()
    {
        ALWAYS_ASSERT(size_, "Removing from empty vector");
        buffer_[--size_].~T();
    }

    constexpr T *const erase(u32 index)
    {
        return erase(buffer_[index]);
    }

    constexpr T *const erase(T *const value)
    {
        ALWAYS_ASSERT(value >= begin() && value < end());
        value->~T();
        --size_;

        if (value < end()) {
            memcpy(value, value + 1, (end() - value) * sizeof(T));
        }

        return value;
    }

    constexpr b8 empty() const { return size_ == 0; }

    constexpr u32 size() const { return size_; }
    constexpr u32 capacity() const { return capacity_; }
    constexpr T* data() { return buffer_; }
    constexpr const T* data() const { return buffer_; }

    constexpr T& at(u32 index) { return buffer_[index]; }
    constexpr const T& at(size_t index) const { return buffer_[index]; }

    constexpr T& front()
    {
        ALWAYS_ASSERT(buffer_ && size_);
        return buffer_[0];
    }
    constexpr const T& front() const
    {
        ALWAYS_ASSERT(buffer_ && size_);
        return buffer_[0];
    }
    constexpr T& back()
    {
        ALWAYS_ASSERT(buffer_ && size_);
        return buffer_[size_ - 1];
    }
    constexpr const T& back() const
    {
        ALWAYS_ASSERT(buffer_ && size_);
        return buffer_[size_ - 1];
    }
    constexpr T* begin() { return buffer_; }
    constexpr const T* begin() const { return buffer_; }
    constexpr T* end() { return buffer_ + size_; }
    constexpr const T* end() const { return buffer_ + size_; }

private:
    constexpr void move(vector<T> &&other)
    {
        clear();
        if (buffer_) { free(buffer_); }

        size_ = other.size_;
        buffer_ = other.buffer_;
        capacity_ = other.capacity_;

        other.size_ = 0;
        other.capacity_ = 0;
        other.buffer_ = nullptr;
    }

private:
    u32 size_ = 0;
    u32 capacity_ = 0;
    T *buffer_ = nullptr;
};

}

#endif // HK_VECTOR_H
