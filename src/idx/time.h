#pragma once

#include "string.h"

namespace hana {

/** Store a time point. */
struct Time {
    int year = 0; // e.g. 2015
    int month = 0; // from 0 to 11
    int day = 0; // from 1 to 31
    int hour = 0; // from 0 to 23
    int minute = 0; // from 0 to 59
    int second = 0; // from 0 to 59
    int milliseconds = 0; // from 0 to 999
};

/** Get the current time. */
void get_current_time(Time& time);

/** Get a formatted string of time in the format Y-M-D H:M:S.m.
\return the number of characters printed. */
int print_formatted_time(const Time& t, StringRef s);

}
