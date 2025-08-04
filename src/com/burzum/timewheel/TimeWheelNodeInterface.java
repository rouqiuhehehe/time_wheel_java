package com.burzum.timewheel;

public abstract class TimeWheelNodeInterface {
    public abstract void run();

    // 定时，单位MS
    protected long delay = 0;
    // 无限循环
    protected long circle = (1L << 32) - 1;
}
