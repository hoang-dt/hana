#include "assert.h"

namespace hana { namespace core {

/** Default failure handler.
Print the error message to the standard output and terminate the program. */
inline void default_callback(const char* condition,
    const char* msg,
    const char* file, int line)
{
    printf("File %s, line %d: Assert failure: ", file, line);
    if (condition) { printf("'%s' ", condition); }
    if (msg) { printf("%s", msg); }
    printf("\n");
}

AssertCallback& callback()
{
    static AssertCallback s_callback = &default_callback;
    return s_callback;
}

}}
