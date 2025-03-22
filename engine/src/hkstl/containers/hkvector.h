#ifndef HK_VECTOR_H
#define HK_VECTOR_H

#include "utility/hktypes.h"
#include "utility/hkassert.h"

#include "math/utils.h"

#include <initializer_list>

// TODO: replace with custom allocators
#include <memory>

namespace hk {

template<typename T>
class vector {
private:
    // Can iterate through container with it, can change value it points to
    using iterator = T*;
    // Can iterate through container with it, can not change value it points to
    using const_iterator = T* const;

public:
    /* ===== Constructors ===== */

    // Constructs a vector with size default-inserted objects of T
    constexpr vector(u32 size = 0);

    // Constructs a vector with count copies of value
    constexpr vector(u32 count, const T &value);

    // Constructs a vector with contents (in-place) of the range [first, last)
    template <typename ItType>
    constexpr vector(ItType first, ItType last);

    // Copy constructor
    constexpr vector(const vector<T> &other);

    // Move constructor
    constexpr vector(vector<T> &&other);

    /* Constructs a vector with the use of initializer list,
     * Same as vector(list.begin(), list.end()) */
    constexpr vector(std::initializer_list<T> list);

    /* ===== Destructors ===== */

    // *sad trumpet noises*
    inline ~vector();

    /* ===== Operator Overloads ===== */

    // Copy assignment operator
    constexpr vector<T>& operator=(const vector<T> &other)
    {
        clear();
        resize(other.size());
        for (u32 i = 0; i < size(); i ++) {
            buffer_[i] = other.buffer_[i];
        }
        return *this;
    }

    // Move assignment operator
    constexpr vector<T>& operator=(vector<T> &&other)
    {
        clear();
        if (buffer_) { free(buffer_); }

        size_ = other.size_;
        buffer_ = other.buffer_;
        capacity_ = other.capacity_;

        other.size_ = 0;
        other.capacity_ = 0;
        other.buffer_ = nullptr;

        return *this;
    }

    // Replaces the contents with the elements from list
    constexpr vector<T>& operator=(std::initializer_list<T> list)
    {
        assign(list);
        return *this;
    }

    // Compares sizes and if each element, at the same position, is equal
    constexpr b8 operator==(const vector<T> &other);
    constexpr b8 operator!=(const vector<T> &other);

    // // Compares the contents lexicographically
    // constexpr b8 operator<(const vector<T> &other);
    // constexpr b8 operator<=(const vector<T> &other);
    //
    // // Compares the contents lexicographically
    // constexpr b8 operator>(const vector<T> &other);
    // constexpr b8 operator>=(const vector<T> &other);

    /* ===== Other Member functions ===== */

    // Replaces the contents with count copies of value
    constexpr void assign(u32 count, const T &value);

    // Replaces the contents with copies of objects in the range [first, last)
    template <typename ItType>
    constexpr void assign(ItType first, ItType last);

    // Replaces the contents with the elements from list
    constexpr void assign(std::initializer_list<T> list);

    /* ===== Element access ===== */

    // Access specified element with bounds checking
    constexpr T& at(u32 index);
    // Access specified element with bounds checking
    constexpr const T& at(u32 index) const;

    // Access specified element without bounds checking
    constexpr T& operator[](u32 index);
    // Access specified element without bounds checking
    constexpr const T& operator[](u32 index) const;

    // Access the first element
    constexpr T& front();
    // Access the first element
    constexpr const T& front() const;

    // Access the last element
    constexpr T& back();
    // Access the last element
    constexpr const T& back() const;

    // Direct access to the underlying contiguous storage
    constexpr T* data();
    // Direct access to the underlying contiguous storage
    constexpr const T* data() const;

    /* ===== Iterators ===== */

    // Returns an iterator to the first element of the container
    constexpr iterator begin();
    // Returns an iterator to the first element of the container
    constexpr const_iterator begin() const;

    // Returns an iterator to the element following the last element
    constexpr iterator end();
    // Returns an iterator to the element following the last element
    constexpr const_iterator end() const;

    /* ===== Capacity ===== */

    // Checks whether the container is empty
    constexpr b8 empty() const;

    // Returns the number of elements
    constexpr u32 size() const;

    // Returns the maximum possible number of elements
    constexpr u32 max_size() const;

    /* Returns the number of elements that can be held
     * in currently allocated storage */
    constexpr u32 capacity() const;

    /* ===== Modifiers ===== */

    // Erases all elements from the container
    constexpr void clear();

    // Inserts a copy of value before pos
    constexpr iterator insert(u32 pos, const T &value);
    // Inserts a copy of value before pos
    constexpr iterator insert(const_iterator pos, const T &value);

    // Inserts value before pos, may use move semantics
    constexpr iterator insert(u32 pos, const T &&value);
    // Inserts value before pos, may use move semantics
    constexpr iterator insert(const_iterator pos, const T &&value);

    // Inserts count copies of the value before pos, count may be zero
    constexpr iterator insert(u32 pos, u32 count, const T &value);
    // Inserts count copies of the value before pos, count may be zero
    constexpr iterator insert(const_iterator pos, u32 count, const T &value);

    /* Constructs element in-place,
     * Inserts a new element into the container directly before pos */
    template <typename... Args>
    constexpr iterator emplace(u32 pos, Args&& ...args);
    /* Constructs element in-place,
     * Inserts a new element into the container directly before pos */
    template <typename... Args>
    constexpr iterator emplace(const_iterator pos, Args&& ...args);

    /* Removes element,
     * Returns iterator following the removed element */
    constexpr iterator erase(u32 pos);
    /* Removes element,
     * Returns iterator following the removed element */
    constexpr iterator erase(const_iterator pos);

    /* Removes elements in range [first, last),
     * Returns iterator following the last removed element */
    constexpr iterator erase(u32 first, u32 last);
    /* Removes elements in range [first, last),
     * Returns iterator following the last removed element */
    constexpr iterator erase(const_iterator first, const_iterator last);

    // Adds an element to the end
    constexpr void push_back(const T &value);
    // Adds an element to the end, may use move semantics
    constexpr void push_back(T &&value);

    /* Constructs an element in-place at the end,
     * Return reference to the inserted element */
    template <typename... Args>
    constexpr T& emplace_back(Args&& ...args);

    // Removes the last element
    constexpr void pop_back();

    /* Increases the capacity of the vector,
     * May reallocate memory, neither creates elements nor changes size */
    constexpr void reserve(u32 capacity);

    /* Changes the number of elements stored,
     * May allocate memory, create/delete elements */
    constexpr void resize(u32 size, const T &value = T());

private:
    template <typename... Args>
    constexpr iterator place(const_iterator pos, u32 count, Args&& ...args);

private:
    u32 size_ = 0;
    u32 capacity_ = 0;
    T *buffer_ = nullptr;
};


/* ===== Constructors ===== */
#define HKVEC_CONSTRACTOR template <typename T> constexpr vector<T>::

HKVEC_CONSTRACTOR vector(u32 size)                  { resize(size); }
HKVEC_CONSTRACTOR vector(u32 count, const T &value) { resize(count, value); }
HKVEC_CONSTRACTOR vector(const vector<T> &other)    { *this = other; }
HKVEC_CONSTRACTOR vector(vector<T> &&other)         { *this = hk::move(other); }

template <typename T>
template <typename ItType>
constexpr vector<T>::vector(ItType first, ItType last)
{
    for (auto it = first; it != last; ++it) {
        emplace_back(*it);
    }
}

template <typename T>
constexpr vector<T>::vector(std::initializer_list<T> list)
    : vector(list.begin(), list.end())
{}

#undef HKVEC_CONSTRACTOR

/* ===== Destructors ===== */

template <typename T>
inline vector<T>::~vector()
{
    clear();
    if (buffer_) { free (buffer_); }
}

/* ===== Operator Overloads ===== */

template <typename T>
constexpr b8 vector<T>::operator==(const vector<T> &other)
{
    if (size_ != other.size()) { return false; }

    const T *lhs_data = buffer_;
    const T *rhs_data = other.data();

    for (u32 i = 0; i < size_; ++i) {
        if (lhs_data[i] != rhs_data[i]) { return false; }
    }

    return true;
}

template <typename T>
constexpr b8 vector<T>::operator!=(const vector<T> &other)
{
    return !(*this == other);
}

/* ===== Other Member functions ===== */

template <typename T>
constexpr void vector<T>::assign(u32 count, const T &value)
{
    clear();

    resize(count, value);
}

template <typename T>
template <typename ItType>
constexpr void vector<T>::assign(ItType first, ItType last)
{
    clear();

    u32 count = static_cast<u32>(last - first);
    if (count > capacity_) {
        reserve(hkm::max(count, (capacity_ + 1) * 2));
    }

    for (auto it = first; it != last; ++it) {
        emplace_back(*it);
    }
}

template <typename T>
constexpr void vector<T>::assign(std::initializer_list<T> list)
{
    assign(list.begin(), list.end());
}

/* ===== Element access ===== */

template <typename T>
constexpr T& vector<T>::at(u32 index)
{
    ALWAYS_ASSERT((size_ > 0 && index < size_), "Out of bounds");
    return buffer_[index];
}

template <typename T>
constexpr const T& vector<T>::at(u32 index) const
{
    ALWAYS_ASSERT((size_ > 0 && index < size_), "Out of bounds");
    return buffer_[index];
}

template <typename T>
constexpr T& vector<T>::operator[](u32 index)
{
    return buffer_[index];
}

template <typename T>
constexpr const T& vector<T>::operator[](u32 index) const
{
    return buffer_[index];
}

template <typename T>
constexpr T& vector<T>::front()
{
    ALWAYS_ASSERT(buffer_ && size_);
    return buffer_[0];
}

template <typename T>
constexpr const T& vector<T>::front() const
{
    ALWAYS_ASSERT(buffer_ && size_);
    return buffer_[0];
}

template <typename T>
constexpr T& vector<T>::back()
{
    ALWAYS_ASSERT(buffer_ && size_);
    return buffer_[size_ - 1];
}

template <typename T>
constexpr const T& vector<T>::back() const
{
    ALWAYS_ASSERT(buffer_ && size_);
    return buffer_[size_ - 1];
}


template <typename T>
constexpr T* vector<T>::data()
{
    return buffer_;
}

template <typename T>
constexpr const T* vector<T>::data() const
{
    return buffer_;
}

/* ===== Iterators ===== */

#define HKVEC_IT typename vector<T>::iterator
#define HKVEC_CONST_IT typename vector<T>::const_iterator

template <typename T>
constexpr HKVEC_IT vector<T>::begin()
{
    return buffer_;
}

template <typename T>
constexpr HKVEC_CONST_IT vector<T>::begin() const
{
    return buffer_;
}

template <typename T>
constexpr HKVEC_IT vector<T>::end()
{
    return buffer_ + size_;
}

template <typename T>
constexpr HKVEC_CONST_IT vector<T>::end() const
{
    return buffer_ + size_;
}

#undef HKVEC_IT
#undef HKVEC_CONST_IT

/* ===== Capacity ===== */

template <typename T>
constexpr b8 vector<T>::empty() const { return size_ == 0; }

template <typename T>
constexpr u32 vector<T>::size() const { return size_; }

template <typename T>
constexpr u32 vector<T>::max_size() const { return static_cast<u32>(-1); }

template <typename T>
constexpr u32 vector<T>::capacity() const { return capacity_; }

/* ===== Modifiers ===== */

#define HKVEC_IT typename vector<T>::iterator
#define HKVEC_CONST_IT typename vector<T>::const_iterator

template <typename T>
constexpr void vector<T>::clear()
{
    for (u32 i = 0; i < size_; i++) {
        buffer_[i].~T();
    }
    size_ = 0;
}

template <typename T>
constexpr HKVEC_IT vector<T>::insert(u32 pos, const T &value)
{
    return insert(begin() + pos, value);
}

template <typename T>
constexpr HKVEC_IT vector<T>::insert(u32 pos, const T &&value)
{
    return insert(begin() + pos, hk::move(value));
}

template <typename T>
constexpr HKVEC_IT vector<T>::insert(u32 pos, u32 count, const T &value)
{
    return insert(begin() + pos, count, value);
}

template <typename T>
constexpr HKVEC_IT vector<T>::insert(HKVEC_CONST_IT pos, const T &value)
{
    return place(pos, 1, value);
}

template <typename T>
constexpr HKVEC_IT vector<T>::insert(HKVEC_CONST_IT pos, const T &&value)
{
    return place(pos, 1, hk::move(value));
}

template <typename T>
constexpr
HKVEC_IT vector<T>::insert(HKVEC_CONST_IT pos, u32 count, const T &value)
{
    return place(pos, count, value);
}

template <typename T>
template <typename... Args>
constexpr HKVEC_IT vector<T>::emplace(u32 pos, Args&& ...args)
{
    return emplace(begin() + pos, hk::forward<Args>(args)...);
}

template <typename T>
template <typename... Args>
constexpr HKVEC_IT vector<T>::emplace(HKVEC_CONST_IT pos, Args&& ...args)
{
    return place(pos, 1, hk::forward<Args>(args)...);
}

template <typename T>
constexpr HKVEC_IT vector<T>::erase(u32 pos)
{
    return erase(begin() + pos);
}

template <typename T>
constexpr HKVEC_IT vector<T>::erase(HKVEC_CONST_IT pos)
{
    return erase(pos, pos + 1);
}

template <typename T>
constexpr HKVEC_IT vector<T>::erase(u32 first, u32 last)
{
    return erase(begin() + first, begin() + last);
}

template <typename T>
constexpr HKVEC_IT vector<T>::erase(HKVEC_CONST_IT first, HKVEC_CONST_IT last)
{
    ALWAYS_ASSERT(first && last, "hkvector: Iterator should no be null");
    ALWAYS_ASSERT(first >= begin() && last <= end(), "hkvector: Out of bounds");

    u32 count = static_cast<u32>(last - first);

    for (HKVEC_IT it = first; it != last; ++it) {
        it->~T();
    }

    // Move remaining elements leftward
    if (last < end()) {
        std::memmove(first, last, static_cast<u32>(end() - first) * sizeof(T));
    }

    size_ -= count;
    return last;
}

template <typename T>
constexpr void vector<T>::push_back(const T &value)
{
    emplace_back(value);
}

template <typename T>
constexpr void vector<T>::push_back(T &&value)
{
    emplace_back(hk::move(value));
}

template <typename T>
template <typename... Args>
constexpr T& vector<T>::emplace_back(Args&& ...args)
{
    if (size_ == capacity_) {
        reserve((capacity_ + 1) * 2);
    }

    new (buffer_ + size_) T(hk::forward<Args>(args)...);
    ++size_;

    return buffer_[size_ - 1];
}

template <typename T>
constexpr void vector<T>::pop_back()
{
    ALWAYS_ASSERT(size_, "hkvector:: Removing from empty container");
    buffer_[--size_].~T();
}

template <typename T>
constexpr void vector<T>::reserve(u32 capacity)
{
    if (capacity <= capacity_) { return; }

    void *tmp = std::realloc(buffer_, capacity * sizeof(T));
    if (!tmp) { return; }

    buffer_ = static_cast<T*>(tmp);
    capacity_ = capacity;
}

template <typename T>
constexpr void vector<T>::resize(u32 size, const T &value)
{
    if (size > size_) {
        reserve(size);
        while (size > size_) {
            emplace_back(std::move(value));
        }
    } else if (size < size_) {
        erase(begin() + size, end());
    }
}

template <typename T>
template <typename... Args>
constexpr
HKVEC_IT vector<T>::place(HKVEC_CONST_IT pos, u32 count, Args&& ...args)
{
    ALWAYS_ASSERT(pos, "hkvector: Iterator should no be null");
    ALWAYS_ASSERT(pos >= begin() && pos <= end(), "hkvector: Out of bounds");

    // FIX: doesn't work when pos == end()

    if (!count) { return begin(); }

    u32 new_size = size_ + count;
    if (new_size >= capacity_) {
        reserve(hkm::max(new_size, (capacity_ + 1) * 2));
    }

    // Shift elements to make room for new elem
    iterator insert_pos = pos; // const, lmao
    u64 shift_count = end() - pos;

    // shift_count == 0, is a valid call
    std::memmove(insert_pos + count, insert_pos, shift_count * sizeof(T));

    for (u32 i = 0; i < count; ++i) {
        new (insert_pos + i) T(hk::forward<Args>(args)...);
    }

    size_ = new_size;
    return insert_pos;
}

#undef HKVEC_IT
#undef HKVEC_CONST_IT

}

#endif // HK_VECTOR_H
