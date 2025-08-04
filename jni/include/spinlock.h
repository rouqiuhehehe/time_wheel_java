//
// Created by Kantu004 on 2025/3/13.
//

#ifndef TIME_WHEEL__SPINLOCK_H_
#define TIME_WHEEL__SPINLOCK_H_

#include <atomic>
#include <thread>

class SpinLock
{
public:
    SpinLock() = default;
    SpinLock(const SpinLock &) = delete;
    SpinLock &operator=(const SpinLock &) = delete;

    void lock() {
        while (lock_.test_and_set(std::memory_order_acquire))
            std::this_thread::yield();
    }

    void unlock() {
        lock_.clear(std::memory_order_release);
    }

    bool try_lock() {
        return !lock_.test_and_set(std::memory_order_acquire);
    }

private:
    std::atomic_flag lock_ = ATOMIC_FLAG_INIT;
};
#endif //TIME_WHEEL__SPINLOCK_H_
