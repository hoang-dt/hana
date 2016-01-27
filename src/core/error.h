#pragma once

namespace hana { namespace core {

/** Store an error message. */
extern thread_local char error_msg[1024];

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

    //virtual const char* get_error_msg() const; // TODO: disabled for not compiling on linux
};

// void set_error_msg(const char* msg); // TODO: disabled for not compiling on linux

}}
