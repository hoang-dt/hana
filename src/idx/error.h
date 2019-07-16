#pragma once

namespace hana {

extern char error_msg[1024];

void set_error_msg(const char* msg);

struct Error {
  enum {
    NoError                = 0,
    FileNotFound           = 1,
    ParsingError           = 2,
    BlockNotFound          = 3,
    HeaderNotFound         = 4,
    HeaderWriteFailed      = 5,
    BlockReadFailed        = 6,
    BlockWriteFailed       = 7,
    CompressionUnsupported = 8,
    InvalidFormat          = 9,
    InvalidHzLevel         = 10,
    InvalidIdxFile         = 11,
    InvalidVolume          = 12,
    InvalidCompression     = 13,
    FieldNotFound          = 14,
    TimeStepNotFound       = 15,
    VolumeTooBig           = 16,
    InvalidGrid            = 17,
    NumErrors
  };
  Error() : code(NoError) {}
  Error(int c) : code(c) {}
  bool operator==(int c) const { return code == c; }
  int code;
  const char* get_error_msg() const;
};

}
