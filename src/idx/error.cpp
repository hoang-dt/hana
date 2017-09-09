#include "error.h"
#include "string.h"

namespace hana {


char error_msg[1024];

void set_error_msg(const char* msg) { copy(STR_REF(error_msg), StringRef(msg)); }

const char* Error::get_error_msg() const
{
  switch (code) {
    case NoError               : return "No error";
    case FileNotFound          : return "File not found";
    case ParsingError          : return "Parsing error";
    case BlockNotFound         : return "Block not found";
    case BlockReadFailed       : return "Block read failed";
    case CompressionUnsupported: return "Compression unsupported";
    case InvalidFormat         : return "Invalid format";
    case InvalidHzLevel        : return "Invalid level";
    case InvalidIdxFile        : return "Invalid idx file";
    case InvalidVolume         : return "Invalid volume";
    case InvalidCompression    : return "Invalid compression";
    case FieldNotFound         : return "Field not found";
    case TimeStepNotFound      : return "Time step not found";
    case VolumeTooBig          : return "Volume too big";
    default                    : return nullptr;
  }
}

}
