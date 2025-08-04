//
// Created by Yoshiki on 2023/6/11.
//

#ifndef LINUX_SERVER_LIB_INCLUDE_PRINTF_COLOR_H_
#define LINUX_SERVER_LIB_INCLUDE_PRINTF_COLOR_H_

#include <cstdarg>
#include <cstring>
#include <iostream>

#define NONE                 "\e[0m"
#define BLACK                "\e[0;30m"
#define L_BLACK              "\e[1;30m"
#define RED                  "\e[0;31m"
#define L_RED                "\e[1;31m"
#define GREEN                "\e[0;32m"
#define L_GREEN              "\e[1;32m"
#define BROWN                "\e[0;33m"
#define YELLOW               "\e[1;33m"
#define BLUE                 "\e[0;34m"
#define L_BLUE               "\e[1;34m"
#define PURPLE               "\e[0;35m"
#define L_PURPLE             "\e[1;35m"
#define CYAN                 "\e[0;36m"
#define L_CYAN               "\e[1;36m"
#define GRAY                 "\e[0;37m"
#define WHITE                "\e[1;37m"

#define BOLD                 "\e[1m"
#define UNDERLINE            "\e[4m"
#define BLINK                "\e[5m"
#define REVERSE              "\e[7m"
#define HIDE                 "\e[8m"
#define CLEAR                "\e[2J"
#define CLRLINE              "\r\e[K" //or "\e[1K\r"

#ifdef __cplusplus
#define STD std::
#else
#define STD
#endif

#define KV_ASSERT(expr) \
    do {                \
        if (unlikely(!(expr))) { \
            PRINT_ERROR("=== ASSERTION FAILED ==="); \
            PRINT_ERROR(#expr " is not true"); \
        }\
    } while(0)

#ifndef NDEBUG
#define PRINT_DEBUG(str, ...) Logger::getInstance().printInfo(Logger::Level::LV_DEBUG, UNDERLINE "%s [%s:%d]" NONE "\t" str "\n", \
                                __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define PRINT_DEBUG(str, ...)
#endif

#ifndef GLOBAL_LOG_LEVEL
#define GLOBAL_LOG_LEVEL Level::LV_NOTICE
#endif

#define SET_LOG_LEVEL(LEVEL) Logger::getInstance().setLogLevel((LEVEL))

#define PRINT_INFO(str, ...) Logger::getInstance().printInfo(Logger::Level::LV_NOTICE, UNDERLINE "%s [%s:%d]" NONE "\t" str "\n", \
                                __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define PRINT_ERROR(str, ...) Logger::getInstance().printErr(UNDERLINE "%s [%s:%d]" NONE "\t" str "\n", \
                                __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define PRINT_WARNING(str, ...) Logger::getInstance().printInfo(Logger::Level::LV_WARNING, BROWN UNDERLINE"%s [%s:%d]" NONE "\t" YELLOW str "\n" NONE, \
                                    __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

namespace Utils
{
    inline std::string getDateNow(const std::string &format = "%Y-%m-%d %H:%M:%S") {
        char time[40];
        auto now = std::chrono::system_clock::now();
        auto currentTime = std::chrono::system_clock::to_time_t(now);

        std::strftime(time, 40, format.c_str(), std::localtime(&currentTime));

        return time;
    }
}
class Logger
{
public:
    enum class Level
    {
        LV_DEBUG,
        LV_NOTICE,
        LV_WARNING,
        LV_ERROR
    };
    ~Logger() noexcept {
        delete[] buf_;
    }
    static Logger &getInstance() {
        static Logger logger;
        return logger;
    }

    static void initLogger(
        std::ostream &out,
        std::ostream &err,
        Level level
    ) {
        getInstance().logLevel = level;
        getInstance().os_ = &out;
        getInstance().err_ = &err;
    }

    void setLogLevel(Level level) {
        logLevel = level;
    }

    void printInfo(Level level, const char *str, ...) {
        if (level >= logLevel) {
            va_list list;
            va_start(list, str);
            sprintf(buf_, "[%s] ", Utils::getDateNow("%Y-%m-%d %H:%M:%S").c_str());
            vsprintf(buf_, str, list);
            *os_ << buf_;
            os_->flush();
            memset(buf_, 0, BUF_SIZE);
            va_end(list);
        }
    }

    void printErr(const char *str, ...) {
        va_list list;
        va_start(list, str);
        sprintf(buf_, "[%s] ", Utils::getDateNow("%Y-%m-%d %H:%M:%S").c_str());
        vsprintf(buf_, str, list);
        *err_ << buf_;
        memset(buf_, 0, BUF_SIZE);
        va_end(list);
    }

private:
    Logger() = default;

private:
    static constexpr size_t BUF_SIZE = 1024 * 1024;
    std::ostream *os_ = &std::cout;
    std::ostream *err_ = &std::cerr;
    char *buf_ = new char[BUF_SIZE];

    Level logLevel = GLOBAL_LOG_LEVEL;
};
#endif //LINUX_SERVER_LIB_INCLUDE_PRINTF_COLOR_H_
