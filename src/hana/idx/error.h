#pragma once

#include "../core/error.h"

namespace hana { namespace idx {

struct Error : public core::Error {
    enum {
        BlockNotFound = core::Error::NumErrors,
        BlockReadFailed,
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

    Error() = default;
    Error(int c) : core::Error(c) {}
    const char* get_error_msg() const;
};

}}
