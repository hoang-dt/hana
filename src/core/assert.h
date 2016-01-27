/** \file
This file defines an assert macro that is more useful than the default one.

It supports the following:
    - formatted error messages, using the printf format (e.g. \%s \%d etc)
    - user-specified error handler (a default one is provided)
    - break into the debugger exactly where the error happens
This code is modified from http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert/
*/

#pragma once

#include "debugbreak.h"
#include <cstdio>

/** \namespace hana::core */
namespace hana { namespace core {

/** The type of a callback function that is called when failure happens. */
using AssertCallback = void (*)(const char* condition,
                                const char* msg,
                                const char* file, int line);

/** Set the error callback to the default one for the first time. From the second
time onward, just return a reference to the current callback. Can be used to set
a new callback as well. */
AssertCallback& callback();

/** Format the given error message and delegate to the actual failure handler. */
template <typename ... Args>
void report_failure(const char* condition,
                    const char* file, int line,
                    const char* format, const Args& ... args)
{
    const char* formatted_msg = nullptr;
    if (format) {
        char msg_buf[1024];
        snprintf(msg_buf, sizeof(msg_buf), format, args ...);
        formatted_msg = msg_buf;
    }
    callback()(condition, formatted_msg, file, line);
}

}} // end namespace hana::core

#define HANA_UNUSED(x) do { (void)sizeof(x); } while(0)

#ifdef HANA_ASSERT_ON
    /** Default assert */
    #define HANA_ASSERT(cond) \
        do \
        { \
            if (!(cond)) { \
                ::hana::core::report_failure(#cond, __FILE__, __LINE__, 0); \
                debug_break(); \
            } \
        } while(0)

    /** Assert and print an error message */
    #define HANA_ASSERT_MSG(cond, msg, ...) \
        do \
        { \
            if (!(cond)) { \
                ::hana::core::report_failure(#cond, __FILE__, __LINE__, (msg), __VA_ARGS__); \
                debug_break(); \
            } \
        } while(0)
#else
    #define HANA_ASSERT(cond) \
        do { HANA_UNUSED(cond); } while(0)
    #define HANA_ASSERT_MSG(cond, msg, ...) \
        do { HANA_UNUSED(cond); HANA_UNUSED(msg); } while(0)
#endif
