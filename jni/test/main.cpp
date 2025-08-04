//
// Created by Kantu004 on 2025/7/14.
//

#include "time_wheel.h"

int main () {
    TimeWheel::initTimeWheel();
    std::cout.setf(std::ios::boolalpha);
    TimeWheel::addTask([](bool isDone, uint32_t id) {
        std::cout << isDone << " id: " << id << std::endl;
    }, 1s, 2);
    TimeWheel::addTask([](bool isDone, uint32_t id) {
        std::cout << isDone << " id: " << id << std::endl;
    }, 1500ms, 4);

    while (true) {}
}