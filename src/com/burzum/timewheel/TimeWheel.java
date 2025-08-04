package com.burzum.timewheel;

public class TimeWheel {
    // 时间刻度，刻度越小越精确
    // 传0调用this_thread::yield() 调度
    public static native void initTimeWheel(long ms);

    public static native <T extends TimeWheelNodeInterface> long addTask(T node);

    public static native void removeTask(long id);

    public static native void stopTimeWheel();
}
