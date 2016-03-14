#include "assert.h"
#include "error.h"
#include "types.h"
#include "string.h"

namespace hana { namespace core {

const char* Error::get_error_msg() const
{
    switch (code) {
        case NoError: return "No error";
        case FileNotFound: return "File not found";
        case ParsingError: return "Parsing error";
        default: return nullptr;
    };
}

char error_msg[1024];

void set_error_msg(const char* msg) { copy(STR_REF(error_msg), StringRef(msg)); }

}}

