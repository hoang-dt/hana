#pragma once

#include <cstdio>
#include <iosfwd>
#include <type_traits>
#include <vector>

namespace hana { namespace core {

template<bool B, typename T = void> using disable_if = std::enable_if<!B, T>;

/** Read (an array of) POD-like variable from a FILE stream */
template <typename T>
typename std::enable_if<std::is_trivially_copyable<T>::value>::type
read(FILE* stream, T* data, size_t count = 1)
{
    fread(data, sizeof(T), count, stream);
}

/** Write (an array of) POD-like variable to a FILE stream */
template <typename T>
typename std::enable_if<std::is_trivially_copyable<T>::value>::type
write(FILE* stream, const T* data, size_t count = 1)
{
    fwrite(data, sizeof(T), count, stream);
}

/** Read (an array of) POD-like variable from a stream */
template <typename T>
typename std::enable_if<std::is_trivially_copyable<T>::value>::type
read(std::istream& is, T* data, size_t count = 1)
{
    is.read(reinterpret_cast<char*>(data), sizeof(T) * count);
}

/** Write (an array of) POD-like variable to a stream */
template <typename T>
typename std::enable_if<std::is_trivially_copyable<T>::value>::type
write(std::ostream& os, const T* data, size_t count = 1)
{
    os.write(reinterpret_cast<const char*>(data), sizeof(T) * count);
}

/** Write a POD-like variable to a stream */
template <typename T>
typename std::enable_if<std::is_trivially_copyable<T>::value>::type
write(std::ostream& os, T data)
{
    write(os, &data, 1);
}

/** Read a std::vector of POD-like variables from a stream */
template <typename T>
typename std::enable_if<std::is_trivially_copyable<T>::value>::type
read(std::istream& is, std::vector<T>* data)
{
    size_t size;
    read(is, &size);
    data->resize(size);
    read(is, &(*data)[0], size);
}

/** Write a std::vector of POD-like variables to a stream */
template <typename T>
typename std::enable_if<std::is_trivially_copyable<T>::value>::type
write(std::ostream& os, const std::vector<T>& data)
{
    write(os, data.size());
    write(os, &(data[0]), data.size());
}

/** Read a std::vector of variables from a stream */
template <typename T>
typename disable_if<std::is_trivially_copyable<T>::value>::type
read(std::istream& is, std::vector<T>* data)
{
    size_t size;
    read(is, &size);
    data->resize(size);
    for (auto& elem : (*data)) {
        read(is, &elem);
    }
}

template <typename T>
typename disable_if<std::is_trivially_copyable<T>::value>::type
write(std::ostream& os, const std::vector<T>& data)
{
    write(os, data.size());
    for (const auto& elem : data) {
        write(os, elem);
    }
}

}}
