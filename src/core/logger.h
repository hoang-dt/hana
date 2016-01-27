#pragma once

#include "macros.h"
#include "assert.h"
#include "time.h"
#include <cstdio>
#include <cstring>
#include <cstdint>

namespace hana { namespace core { namespace logger {

static const int max_num_channels_ = 32;

/** A message with a log level larger than the currently set log level of the
system will get logged. */
enum LogLevel { Fatal, Error, Warning, Info, Debug, Trace, NumLevels };

namespace detail {

/** Literal strings for the LogLevel enum. */
extern char* log_level_str[];

/** A log channel has a name and an output stream (e.g. stdout). */
struct LogChannel {
    const char* name = nullptr; /** Name of the channel. */
    FILE* output = nullptr; /** Output stream of the channel. */
};

/** Get/set a log channel. */
LogChannel& channel(int cid);

} // end namespace detail

void enable_channel(int cid);
void disable_channel(int cid);
bool is_channel_active(int cid);

/** Open a file stream as the output for the given log channel. */
bool open_channel_output(int cid, const char* file_name);
void close_channel_output(int cid);

/** Get/set the output of a log channel. */
FILE*& channel_output(int cid);

const char*& channel_name(int cid);

/** Get the current log level. Can also be used to change it. */
LogLevel& log_level();

/** Log a message. */
template <typename ... Args>
void log(LogLevel level, int cid, const char* format, const Args& ... args)
{
    HANA_ASSERT(level >=0 && level < LogLevel::NumLevels);
    if (level <= log_level() && is_channel_active(cid)) {
        detail::LogChannel& channel = detail::channel(cid);
        HANA_ASSERT(channel.name && channel.output);
        char buffer[1024];
        Time t;
        get_current_time(t);
        int nc = print_formatted_time(t, STR_REF(buffer));
        nc += snprintf(buffer + nc, sizeof(buffer) - nc,
                       " [%s] (%s): ", channel.name, detail::log_level_str[level]);
        snprintf(buffer + nc, sizeof(buffer) - nc, format, args ...);
        fprintf(channel.output, "%s", buffer);
    }
}

/** Shortcut for the more general function of the same name. */
template <typename ... Args>
void log(const char* format, const Args& ... args)
{
    log(Info, 0, format, std::forward<const Args&>(args)...);
}

// TODO: provide the << interface

}}} // end namespace hana::core::log
