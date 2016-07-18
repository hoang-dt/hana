#include "macros.h"
#include "assert.h"
#include "bitops.h"
#include "logger.h"
#include <cstdint>

namespace hana {

namespace detail {
const char* log_level_str[] = { "Fatal", "Error", "Warning", "Info", "Debug", "Trace" };

LogChannel& channel(int cid)
{
    HANA_ASSERT(cid >= 0 && cid < max_num_channels_);
    static LogChannel channels[max_num_channels_];
    return channels[cid];
}

uint32_t& active_channels()
{
    static uint32_t active_channels = 0;
    return active_channels;
}
}

void enable_channel(int cid)
{
    HANA_ASSERT(cid >= 0 && cid < max_num_channels_);
    set_bit(detail::active_channels(), cid);
    auto& channel = detail::channel(cid);
    if (!channel.name) {
        channel.name = "App";
    }
    if (!channel.output) {
        channel.output = stdout;
    }
}

void disable_channel(int cid)
{
    HANA_ASSERT(cid >= 0 && cid < max_num_channels_);
    unset_bit(detail::active_channels(), cid);
}

bool is_channel_active(int cid)
{
    HANA_ASSERT(cid >= 0 && cid < max_num_channels_);
    return check_bit(detail::active_channels(), cid);
}

bool open_channel_output(int cid, const char* file_name)
{
    HANA_ASSERT(file_name);
    detail::channel(cid).output = fopen(file_name, "w");
    return detail::channel(cid).output != nullptr;
}

void close_channel_output(int cid)
{
    FILE* output = detail::channel(cid).output;
    HANA_ASSERT(output != stdout && output != stderr);
    if (output) {
        fclose(output);
        output = nullptr;
    }
}

FILE*& channel_output(int cid)
{
    return detail::channel(cid).output;
}

const char*& channel_name(int cid)
{
    return detail::channel(cid).name;
}

LogLevel& log_level()
{
    static LogLevel level = LogLevel::Info;
    return level;
}

}
