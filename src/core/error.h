#pragma once

namespace hana { namespace core {

/** Store an error message. */
extern char error_msg[1024];

struct Error {
    enum {
        NoError,
        FileNotFound,
        ParsingError,
        NumErrors
    };
    Error() : code(NoError) {}
    Error(int c) : code(c) {}
    bool operator==(int c) const { return code == c; }
    int code;

    const char* get_error_msg() const;
};

void set_error_msg(const char* msg);

}}
