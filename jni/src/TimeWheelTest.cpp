//
// Created by Kantu004 on 2025/3/13.
//

#define GLOBAL_LOG_LEVEL Logger::Level::LV_DEBUG

#include "com_burzum_timewheel_TimeWheel.h"
#include "time_wheel.h"
#include "printf-color.h"


/*
int	"I"
boolean	"Z"
byte	"B"
char	"C"
short	"S"
long	"J"
float	"F"
double	"D"
void	"V"

int[]	"[I"
String[]	"[Ljava/lang/String;"
 */

static JavaVM *jvm;
// static jclass iCallBackClazz;

static JNIEnv *global_env;

static TimeWheel::TimeType getNow() {
    return std::chrono::duration_cast <TimeWheel::TimeType>(std::chrono::system_clock::now().time_since_epoch());
}

jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    // 把 JavaVM 存下来，在回调函数中通过它来得到 JNIEnv
    jvm = vm;
    PRINT_DEBUG("jvm : %p", jvm);
    std::cout << "jvm : " << jvm << std::endl;
    // JNIEnv *env;
    // if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_8) != JNI_OK) {
    //     return JNI_ERR;
    // }
    //
    // // 把回调函数的 jclass 通过 GlobalRef 的方式存下来，因为 jclass (是 Local Reference)不能跨线程
    // iCallBackClazz = (jclass) env->NewGlobalRef(env->FindClass("component/TimeWheelNodeInterface"));
    // std::cout << iCallBackClazz << std::endl;
    return JNI_VERSION_1_8;
}

JNIEXPORT void JNICALL Java_com_burzum_timewheel_TimeWheel_initTimeWheel(JNIEnv *, jclass, jlong ms) {
    auto scale = TimeWheel::TimeType(ms);
    TimeWheel::initTimeWheel(scale);
    PRINT_DEBUG("init time wheel success : scale %ld ms", scale.count());
}

JNIEXPORT jlong JNICALL Java_com_burzum_timewheel_TimeWheel_addTask(JNIEnv *env, jclass thiz, jobject node) {
    PRINT_DEBUG("call addTask");
    auto globalPtr = env->NewGlobalRef(node);
    jclass jclazz = env->GetObjectClass(node);
    jfieldID field = env->GetFieldID(jclazz, "delay", "J");
    if (!field) {
        PRINT_ERROR("field delay is nullptr.");
        return JNI_ERR;
    }
    jlong delay = env->GetLongField(node, field);
    PRINT_DEBUG("delay : %ld", delay);
    field = env->GetFieldID(jclazz, "circle", "J");
    if (!field) {
        PRINT_ERROR("field circle is nullptr.");
        return JNI_ERR;
    }
    jlong circle = env->GetLongField(node, field);
    PRINT_DEBUG("circle : %ld", circle);

    field = env->GetFieldID(jclazz, "immediate", "Z");
    jboolean immediate;
    if (!field) {
        immediate = false;
    } else {
        immediate = env->GetBooleanField(node, field);
    }

    return (jlong) TimeWheel::addTask(
        [=](bool isDone, size_t id) {
            PRINT_DEBUG("run task : %ld", id);
            if (!global_env) {
                jvm->AttachCurrentThread(reinterpret_cast<void **>(&global_env), nullptr);
                PRINT_DEBUG("global_env: %p", globalPtr);
            }
            jclass jclazz = global_env->GetObjectClass(globalPtr);
            if (!jclazz) {
                PRINT_ERROR("jclazz is nullptr.");
                return JNI_ERR;
            }
            jmethodID jmethodId = global_env->GetMethodID(jclazz, "run", "()V");
            if (!jmethodId) {
                PRINT_ERROR("jmethodId run() is nullptr.");
                return JNI_ERR;
            }
            PRINT_DEBUG("globalPtr : %p", globalPtr);
            global_env->CallVoidMethod(globalPtr, jmethodId);
            if (isDone) {
                PRINT_DEBUG("task : %ld is done", id);
                global_env->DeleteGlobalRef(globalPtr);
            }
            return JNI_OK;
        }, TimeWheel::TimeType(delay), circle, immediate
    );
}
JNIEXPORT void JNICALL Java_com_burzum_timewheel_TimeWheel_removeTask(JNIEnv *, jclass, jlong id) {
    TimeWheel::eraseTask(id);
}

JNIEXPORT void JNICALL Java_com_burzum_timewheel_TimeWheel_stopTimeWheel(JNIEnv *, jclass) {
    TimeWheel::stopTimeWheel();
    jvm->DetachCurrentThread();
}
