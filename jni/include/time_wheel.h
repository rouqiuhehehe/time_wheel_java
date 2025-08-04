//
// Created by Kantu004 on 2025/3/12.
//

#ifndef TIME_WHEEL__TIME_WHEEL_H_
#define TIME_WHEEL__TIME_WHEEL_H_

// #include "spinlock.h"
#include <list>
#include <functional>
#include <chrono>
#include <thread>
#include <iostream>
#include "spinlock.h"

#ifdef __linux__
#include <pthread.h>
#elif defined(_WIN32)

#include <windows.h>

#endif

#define LEVEL 4
#define TIME_NEAR_SHIFT 8
#define TIME_NEAR (1 << TIME_NEAR_SHIFT)
#define TIME_LEVEL_SHIFT 6
#define TIME_LEVEL (1 << TIME_LEVEL_SHIFT)
#define TIME_NEAR_MASK (TIME_NEAR - 1)
#define TIME_LEVEL_MASK (TIME_LEVEL - 1)

#define OFFSET(N) (TIME_NEAR + (N) * TIME_LEVEL)
#define INDEX(V, N) ((V >> (TIME_NEAR_SHIFT + (N) * TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK)

using namespace std::chrono_literals;
class TimeWheel
{
    class TimeNode;
public:
    using FunctionType = std::function <void(bool, size_t)>;
    using TimeType = std::chrono::milliseconds;
    using TimeList = std::list <TimeNode>;
    static constexpr uint32_t FULL_CIRCLE = std::numeric_limits <uint32_t>::max();
    TimeWheel() = delete;
    TimeWheel(const TimeWheel &) = delete;
    TimeWheel &operator=(const TimeWheel &) = delete;
    ~TimeWheel() = default;

    // 一个刻度的时间，默认100ms
    static void initTimeWheel(TimeType scale = 100ms) {
        if (!init_) {
            spinLock_.lock();
            if (!init_) {
                init_ = true;
                scale_ = scale;
                jthread_ = std::jthread(&TimeWheel::runTimeWheel);
                // setThreadAffinity();
                current_ = getNow();
            }
            spinLock_.unlock();
        }
    }

    static void stopTimeWheel() {
        if (spinLock_.try_lock()) {
            jthread_.request_stop();
            jthread_.join();
            init_ = false;
            idMap_.clear();
            for (auto &v: tv_) {
                v.clear();
            }
            spinLock_.unlock();
        }
    }

    template <class F>
    static size_t addTask(F &&fn, const TimeType expire, const uint32_t circleCount = FULL_CIRCLE, bool immediate = false) {
        TimeNode timeNode;
        timeNode.fn_ = std::forward <F>(fn);
        timeNode.expire_ = getNow() + (immediate ? TimeType(0) : expire);
        timeNode.circleCount_ = circleCount;
        timeNode.interval_ = expire;

        addNode(timeNode);

        return timeNode.id_;
    }

    static bool eraseTask(const uint32_t id) {
        spinLock_.lock();
        auto ret = idMap_.find(id);
        if (ret == idMap_.end()) {
            std::cerr << "error: cant find erase task id\n";
            return false;
        }
        ret->second->list_->erase(ret->second);
        idMap_.erase(ret);
        spinLock_.unlock();
        return true;
    }

private:
    static void setThreadAffinity() {
#ifdef __linux__
        cpu_set_t cpuSet;
        CPU_ZERO(&cpuSet);
        CPU_SET(0, &cpuSet);
        if (pthread_setaffinity_np(jthread_.native_handle(), sizeof(cpu_set_t), &cpuSet) != 0) {
            std::cerr << "set thread affinity failed";
        }
#elif defined(_WIN32)
        SetThreadAffinityMask(reinterpret_cast<HANDLE>(jthread_.native_handle()), 0x1);
#endif
    }

    static void runTimeWheel(const std::stop_token &stopToken) {
        current_ = getNow();
        while (!stopToken.stop_requested()) {
            check();
            if (scale_.count()) {
                std::this_thread::sleep_for(scale_);
            } else {
                std::this_thread::yield();
            }
        }
    }

    static void addNode(TimeNode &node) {
        size_t idx = (node.expire_ - current_).count();
        if (idx < TIME_NEAR) {
            idx = node.expire_.count() & TIME_NEAR_MASK;
        } else if (idx < (1 << (TIME_NEAR_SHIFT + TIME_LEVEL_SHIFT))) {
            idx = OFFSET(0) + INDEX(node.expire_.count(), 0);
        } else if (idx < (1 << (TIME_NEAR_SHIFT + 2 * TIME_LEVEL_SHIFT))) {
            idx = OFFSET(1) + INDEX(node.expire_.count(), 1);
        } else if (idx < (1 << (TIME_NEAR_SHIFT + 3 * TIME_LEVEL_SHIFT))) {
            idx = OFFSET(2) + INDEX(node.expire_.count(), 2);
        } else if ((long long) idx < 0) {
            // 用于处理定时器调度中因为时间误差导致的过期任务
            // 以及scale_精度导致的过期任务
            idx = current_.count() & TIME_NEAR_MASK;
        } else {
            if (idx > std::numeric_limits <uint32_t>::max()) {
                idx = std::numeric_limits <uint32_t>::max();
                node.expire_ = std::chrono::milliseconds(idx) + current_;
            }
            idx = OFFSET(3) + INDEX(node.expire_.count(), 3);
        }

        spinLock_.lock();
        TimeList &timeList = tv_[idx];
        node.list_ = &timeList;
        timeList.emplace_back(node);
        idMap_[node.id_] = timeList.rbegin().base();
        spinLock_.unlock();
    }

    static void check() {
        // spinLock_.lock();
        auto now = getNow();
        while (current_ <= now) {
            size_t idx = current_.count() & TIME_NEAR_MASK;

            if (!idx && !cascade(OFFSET(0), INDEX(current_.count(), 0)) &&
                !cascade(OFFSET(1), INDEX(current_.count(), 1)) &&
                !cascade(OFFSET(2), INDEX(current_.count(), 2)))
                cascade(OFFSET(3), INDEX(current_.count(), 3));

            ++current_;

            auto temp = std::move(tv_[idx]);
            bool isDone;
            for (auto &v: temp) {
                // 如果还有需要执行的次数，重新计算过期时间并添加进时间轮调度
                if (v.circleCount_ == FULL_CIRCLE || --v.circleCount_) {
                    v.expire_ = now + v.interval_;
                    isDone = false;
                    addNode(v);
                } else {
                    isDone = true;
                    idMap_.erase(v.id_);
                }
                v.fn_(isDone, v.id_);
            }
        }
    }

    static bool cascade(int offset, int idx) {
        TimeList &list = tv_[offset + idx];
        TimeList temp = std::move(list);

        for (auto &v: temp) {
            addNode(v);
        }

        return idx;
    }

    static TimeType getNow() {
        return std::chrono::duration_cast <TimeType>(std::chrono::system_clock::now().time_since_epoch());
    }

    struct TimeNode
    {
        FunctionType fn_;
        TimeType expire_;
        TimeType interval_;
        uint32_t circleCount_;
        TimeList *list_;
        const size_t id_ = getId();

    private:
        static size_t getId() {
            static size_t id = 0;
            return id++;
        };
    };

    static inline std::unordered_map <size_t, TimeList::iterator> idMap_;
    static inline TimeType current_;
    static inline TimeType scale_;
    static inline bool init_ = false;
    static inline std::jthread jthread_;
    static inline SpinLock spinLock_;
    static inline TimeList tv_[TIME_NEAR + LEVEL * TIME_LEVEL];
};
#endif //TIME_WHEEL__TIME_WHEEL_H_