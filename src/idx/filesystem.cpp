#include "macros.h"
#include "assert.h"
#include "filesystem.h"
#include "string.h"
#include <iostream>

#ifdef _WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else // #ifdef _WIN32
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

namespace hana {

Path::Path()
{
  buffer_[0] = '\0';
  components_[0] = StringRef(buffer_, 1);
}

Path::Path(StringRef path_str)
{
  buffer_[0] = '\0';
  construct_from(path_str);
}

Path::Path(const Path& other) : PathBase(other)
{
  for (size_t i = 0; i < num_components_; ++i) {
    components_[i].ptr = buffer_ + (other.components_[i].cptr - other.buffer_);
    components_[i].size = other.components_[i].size;
  }
}

Path& Path::operator=(const Path& other)
{
  detail::PathBase::operator=(other);
  for (size_t i = 0; i < num_components_; ++i) {
    components_[i].ptr = buffer_ + (other.components_[i].cptr - other.buffer_);
    components_[i].size = other.components_[i].size;
  }
  return *this;
}

void Path::construct_from(StringRef path_str)
{
  copy(STR_REF(buffer_), path_str);
  StringTokenizer tokenizer(buffer_, '/');
  StringRef token = tokenizer.next();
  size_t i = 0;
  while (token) {
    components_[i++] = token;
    token = tokenizer.next();
  }
  HANA_ASSERT(i < ARRAY_SIZE(components_));
  // the last component acts as a sentinel, it starts with a pointer to the
  // NULL character in the underlying buffer and have a size of 1
  if (i > 0) {
    components_[i] = StringRef(components_[i - 1].ptr + components_[i - 1].size, 1);
  }
  num_components_ = i;
}

bool Path::is_relative() const
{
  return is_relative_path(STR_REF(buffer_));
}

StringRef Path::last() const
{
  return num_components_ > 0 ? components_[num_components_ - 1] : StringRef();
}

StringRef Path::first() const
{
  return num_components_ > 0 ? components_[0] : StringRef();
}

size_t Path::num_components() const
{
  return num_components_;
}

StringRef Path::operator[](size_t i) const
{
  HANA_ASSERT(i < ARRAY_SIZE(components_));
  return components_[i];
}

const StringRef* Path::begin() const
{
  return components_;
}

const StringRef* Path::end() const
{
  return components_ + num_components_;
}

StringRef Path::path_string() const
{
  return StringRef(buffer_);
}

void Path::add_component(StringRef component)
{
  while (component.size > 0 && start_with(component, STR_REF("/"))) {
    component = StringRef(component.ptr + 1, component.size - 1);
  }
  // add the "/" if this is not the first component
  if (num_components_ > 0) {
    *(components_[num_components_].ptr++) = '/';
  }
  components_[num_components_].size = component.size;
  copy(components_[num_components_], component);
  size_t i = ++num_components_;
  components_[i] = StringRef(components_[i - 1].ptr + components_[i - 1].size, 1);
}

void Path::append(const Path& other)
{
  for (auto& component : other) {
    add_component(component);
  }
}

void Path::remove_last()
{
  if (num_components_ > 0) {
    auto ptr = components_[--num_components_].ptr;
    if (ptr != buffer_) { // guard against writing past the beginning of the buffer
      *(--ptr) = '\0';
    }
  }
}

std::ostream& operator<<(std::ostream& os, const Path& path)
{
  os << path.path_string();
  return os;
}

bool get_current_dir(StringRef path)
{
  if (!GetCurrentDir(path.ptr, static_cast<int>(path.size))) {
    return false;
  }
  return true;
}

bool is_relative_path(StringRef path)
{
  if (path.size > 0 && path[0] == '/') { // e.g. /usr/local
    return false;
  }
  if (path.size > 2 && path[1] == ':' && path[2] == '/') { // e.g. C:/Users
    return false;
  }
  return true;
}

#include <windows.h>
bool create_full_dir(StringRef path)
{
  char path_copy[MAX_PATH];
  strncpy(path_copy, path.cptr, MAX_PATH-1);
  path_copy[MAX_PATH-1] = '\0';
  //mode_t nMode = 0733; // UNIX style permissions
  int error = 0;
#if defined(_WIN32)
  char* p = path_copy;
  for (p = strchr(path_copy, '/'); p; p = strchr(p+1, '/')) {
    *p = '\0';
    error = _mkdir(path_copy); // can be used on Windows
    *p = '/';
  }
  error = _mkdir(path_copy);
#else
  error = mkdir(path_copy.c_str(), nMode); // can be used on non-Windows
#endif
  return (error == 0);
}

} // namespace hana
