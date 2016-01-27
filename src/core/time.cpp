#include "time.h"
#include <cstdio>

#ifdef _WIN32
#include "windows.h"

void hana::core::get_current_time(hana::core::Time& time)
{
    SYSTEMTIME t;
    GetLocalTime(&t);
    time.year = t.wYear;
    time.month = t.wMonth;
    time.day = t.wDay;
    time.hour = t.wHour;
    time.minute = t.wMinute;
    time.second = t.wSecond;
    time.milliseconds = t.wMilliseconds;
}
#elif defined __linux__ || defined __APPLE__
#include <sys/time.h>
void hana::core::get_current_time(hana::core::Time& time)
{
    timeval cur_time;
    gettimeofday(&cur_time, nullptr);
    tm* local_time = localtime(&cur_time.tv_sec);
    time.year = local_time->tm_year + 1990;
    time.month = local_time->tm_mon;
    time.day = local_time->tm_mday;
    time.hour = local_time->tm_hour;
    time.minute = local_time->tm_min;
    time.second = local_time->tm_sec;
    time.milliseconds = cur_time.tv_usec / 1000;
}
#endif

namespace hana { namespace core {
int print_formatted_time(const Time& t, StringRef s)
{
    return snprintf(s.ptr, s.size, "%4d-%02d-%02d %02d:%02d:%02d.%03d",
        t.year, t.month, t.day, t.hour, t.minute, t.second, t.milliseconds);
}
}}
