#ifndef HK_THREAD_H
#define HK_THREAD_H

#include "defines.h"

#include <memory>

namespace hk {

class thread {
public:
    constexpr thread() = delete;

    constexpr thread(const thread &other) = delete;
    constexpr thread(thread &&other) { move(std::move(other)); }

    inline ~thread()
    {
    }

    constexpr thread& operator=(const thread &other) = delete;
    constexpr thread& operator=(thread &&other)
    {
        move(static_cast<thread&&>(other));
        return *this;
    }

    void init();
    void deinit();

    void join();
    void detach();

private:
    constexpr void move(thread &&other)
    {
    }

private:
    i32 id_ = 0;
    i32 handle_ = 0;
};

}

#endif // HK_THREAD_H
