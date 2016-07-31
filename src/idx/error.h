#pragma once

namespace hana {

extern char error_msg[1024];

void set_error_msg(const char* msg);

struct Error {
    enum {
        NoError,
        FileNotFound,
        ParsingError,
        BlockNotFound,
        HeaderNotFound,
        BlockReadFailed,
        BlockWriteFailed,
        CompressionUnsupported,
        InvalidFormat,
        InvalidHzLevel,
        InvalidIdxFile,
        InvalidVolume,
        InvalidCompression,
        FieldNotFound,
        TimeStepNotFound,
        VolumeTooBig,
        NumErrors
    };
    Error() : code(NoError) {}
    Error(int c) : code(c) {}
    bool operator==(int c) const { return code == c; }
    int code;
    const char* get_error_msg() const;
};

}
