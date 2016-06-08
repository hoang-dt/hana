#include "constants.h"
#include "math.h"
#include "string.h"
#include "types.h"
#include "utils.h"
#include <cstddef>
#include <cstring>
#include <ostream>

namespace hana { namespace core {

StringRef::StringRef(const char* p) : cptr(p), size(strlen(p)) {}

std::ostream& operator<<(std::ostream& os, const StringRef& str)
{
    for (char c : str) {
        os << c;
    }
    return os;
}

void copy(StringRef dst, StringRef src, bool add_null)
{
    HANA_ASSERT(src && dst);
    HANA_ASSERT(src.size <= dst.size);
    memcpy(dst.ptr, src.ptr, src.size);
    // we assume that the storage for a StringRef has enough space to put the
    // null character at the end
    if (add_null) {
        dst.ptr[src.size] = '\0';
    }
}

StringRef sub_string(StringRef str, size_t begin, size_t size)
{
    if (!str || begin >= str.size) {
        return StringRef();
    }
    return StringRef(str.ptr + begin, min(size, str.size));
}

StringRef sub_string(StringRef str, size_t begin)
{
    return sub_string(str, begin, str.size - begin);
}

size_t find(StringRef str, StringRef sub)
{
    for (size_t i = 0; i + sub.size <= str.size; ++i) {
        bool found = true;
        for (size_t j = 0; j < sub.size; ++j) {
            if (str[i + j] != sub[j]) {
                found = false;
                break;
            }
        }
        if (found) {
            return i;
        }
    }
    return size_max_;
}

size_t find_last(StringRef str, StringRef sub)
{
    if (sub.size > str.size) {
        return size_max_;
    }
    /* comparing size_t i >= 0 is always true which leads to possible memory
       access outside the string str memory bounds */
    for (size_t i = str.size - sub.size + 1; i--;) {
        bool found = true;
        for (size_t j = 0; j < sub.size; ++j) {
            if (str[i + j] != sub[j]) {
                found = false;
                break;
            }
        }
        if (found) {
            return i;
        }
    }
    return size_max_;
}

size_t replace(StringRef str, char old_c, char new_c)
{
    size_t num_replaced = 0;
    for (auto& c : str) {
        if (c == old_c) {
            c = new_c;
            ++num_replaced;
        }
    }
    return num_replaced;
}

bool operator==(StringRef a, StringRef b)
{
    if (!a || !b) {
        return false;
    }
    if (a.size != b.size) {
        return false;
    }
    for (size_t i = 0; i < a.size; ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

bool operator==(StringRef a, const char* b)
{
    return operator==(a, StringRef(b));
}

bool start_with(StringRef str, StringRef sub)
{
    return sub_string(str, 0, sub.size) == sub;
}

bool to_int(StringRef str, int& val)
{
    if (!str) {
        return false;
    }

    val = 0;
    int multiplier = str[0] == '-' ? -1 : 1;
    for (int i = 0; i < str.size; ++i) {
        unsigned int v = str[str.size - i - 1] - '0';
        if (v < 10) {
            val += multiplier * ((int)v * int(pow10[i]));
        }
        else if ((i + 1 != str.size) || (multiplier != -1)) {
            return false;
        }
    }
    return true;
}

StringTokenizer::StringTokenizer(StringRef input, char delim)
    : str_(input)
    , delim_(delim) {}

StringTokenizer::StringTokenizer(const char* input, char delim)
    : str_(StringRef(input, strlen(input)))
    , delim_(delim) {}

StringRef StringTokenizer::next()
{
    StringRef token;
    while (pos_ < str_.size && str_[pos_] == delim_) {
        ++pos_;
    }
    if (pos_ < str_.size) {
        token.ptr = str_.ptr + pos_;
    }
    while (pos_ < str_.size && str_[pos_++] != delim_) {
        ++token.size;
    }
    return token;
}

void StringTokenizer::reset() { pos_ = 0; }

}} // end namespace hana::core
