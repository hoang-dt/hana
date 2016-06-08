#include "error.h"

namespace hana { namespace idx {


const char* Error::get_error_msg() const
{
    const char* msg = core::Error::get_error_msg();
    if (!msg) {
        switch (code) {
            case BlockNotFound: return "Block not found";
            case BlockReadFailed: return "Block read failed";
            case CompressionUnsupported: return "Compression unsupported";
            case InvalidFormat: return "Invalid format";
            case InvalidHzLevel: return "Invalid level";
            case InvalidIdxFile: return "Invalid idx file";
            case InvalidVolume: return "Invalid volume";
            case InvalidCompression: return "Invalid compression";
            case FieldNotFound: return "Field not found";
            case TimeStepNotFound: return "Time step not found";
            case VolumeTooBig: return "Volume too big";
            default: return nullptr;
        }
    }
    return msg;
}

}}
