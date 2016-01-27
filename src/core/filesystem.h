#pragma once

#include "string.h"
#include <cstdio>
#include <iosfwd>

#ifdef _WIN32
//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>
#define PATH_MAX 260 // a hack to avoid having to #include <Windows.h>
#elif defined __linux__
#include <linux/limits.h>
#include <unistd.h>
// TODO: how aabout mac?
#endif

namespace hana { namespace core {

/** Only support the forward slash '/' separator. */
class Path {
  private:
    /** For example, /home/dir/file.txt */
    char buffer_[PATH_MAX];
    /** e.g. home, dir, file.txt */
    StringRef components_[PATH_MAX / 2 + 1] = {};
    size_t num_components_ = 0;

  public:
    Path();
    explicit Path(StringRef str);

    /** Construct a Path from a StringRef without using a constructor. */
    void construct_from(StringRef str);

    bool is_relative() const;

    /** Get the i(th) component. */
    StringRef operator[](size_t i) const;

    /** Return the last component if it is not followed by '/'. Otherwise return
    an empty StringRef. */
    StringRef last() const;
    /** Return the first component, without the '/' part. */
    StringRef first() const;

    size_t num_components() const;

    const StringRef* begin() const;
    const StringRef* end() const;

    /** Add a component to the end (e.g. "C:/Users" + "Meow" = "C:/Users/Meow"). */
    void add_component(StringRef component);

    /** Append another path to the end. */
    void append(const Path& other);

    /** Remove the last component, useful for removing the file name at the end
    of a path. */
    void remove_last();

    StringRef path_string() const;
};

std::ostream& operator<<(std::ostream& os, const Path& path);

/** Get the directory where the program is launched from. */
bool get_current_dir(StringRef path);

/** Return true only if the path is relative. */
bool is_relative_path(StringRef path);

}}
