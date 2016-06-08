#pragma once

#include "assert.h"
#include <iosfwd>

/** Useful to create a StringRef out of a literal string in a fast way. */
#define STR_REF(x) hana::core::StringRef((x), sizeof(x) - 1)

namespace hana { namespace core {

/** A "view" into a (usually bigger) null-terminated string. A StringRef itself
is not null-terminated.
There are two preferred ways to construct a StringRef from an char[] array. If
you prefer the StringRef to refer to the entire array, use the macro STR_REF
defined above. If you prefer the StringRef to only refer up to the first NULL
character in the array, use the StringRef(const char* p) constructor.
To create a StringRef that encapsulates a literal string (e.g. "Hello"), use the
STR_REF macro. */
struct StringRef {
    union {
        char* ptr = nullptr;
        const char* cptr;
    };

    size_t size = 0;

    StringRef() : ptr(nullptr), size(0) {}
    StringRef(char* p, size_t s) : ptr(p), size(s) {}
    StringRef(const char* p, size_t s) : cptr(p), size(s) {}
    explicit StringRef(const char* p);
    char& operator[](size_t i) { HANA_ASSERT(i < size); return ptr[i]; }
    char operator[](size_t i) const { HANA_ASSERT(i < size); return ptr[i]; }
    char* begin() { return ptr; }
    char* end() { return ptr + size; }
    const char* begin() const { return cptr; }
    const char* end() const { return cptr + size; }
    operator bool() const { return ptr != nullptr; }
};

/** Tokenize a string. */
// TODO: make the delimiter a string?
class StringTokenizer {
private:
    StringRef str_;
    char delim_ = ' ';
    size_t pos_ = 0;

public:
    StringTokenizer(StringRef input, char delim = ' ');
    StringTokenizer(const char* input, char delim = ' ');
    StringRef next();
    void reset();
};

std::ostream& operator<<(std::ostream& os, const StringRef& str);

/** Copy the underlying buffer referred to by src to the one referred to by dst.
add_null should be true whenever dst represents a whole string (as opposed to a
sub-string). The size of src must not be larger than the size of dst.
IMPORTANT: dst itself will not change, only the underlying buffer does. */
void copy(StringRef dst, StringRef src, bool add_null = true);

/** Return a substring of a given string.
The substring starts at begin and has size size.
Return the empty string if the parameters are such that no proper substring can
be constructed (e.g. begin >= str.size).
Return the whole string if begin + size >= str.size.
*/
StringRef sub_string(StringRef str, size_t begin, size_t size);

/** Return a substring from begin to the end. */
StringRef sub_string(StringRef str, size_t begin);

/** Return the first index at which a substring occurs.
Return (size_t)-1 if not found. */
size_t find(StringRef str, StringRef sub);

/** Similar to find_sub_string but find the last occurrence. */
size_t find_last(StringRef str, StringRef sub);

/** Replace all occurrences of old_c with new_c. Return the number of characters
replaced. */
size_t replace(StringRef str, char old_c, char new_c);

bool operator==(StringRef a, StringRef b);

bool operator==(StringRef a, const char* b);

bool start_with(StringRef str, StringRef sub);

bool to_int(StringRef str, int& val);

}}
