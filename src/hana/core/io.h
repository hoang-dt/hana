#pragma once

#include <cstdio>
#include <iosfwd>
#include <type_traits>
#include <vector>

namespace hana { namespace core {

template <typename T>
void read(FILE* stream, T* data, size_t count = 1)
{
    fread(data, sizeof(T), count, stream);
}

template <typename T>
void write(FILE* stream, const T* data, size_t count = 1)
{
    fwrite(data, sizeof(T), count, stream);
}

template <typename T>
void read(std::istream& is, T* data, size_t count = 1)
{
    is.read(reinterpret_cast<char*>(data), sizeof(T) * count);
}

template <typename T>
void write(std::ostream& os, const T* data, size_t count = 1)
{
    os.write(reinterpret_cast<const char*>(data), sizeof(T) * count);
}

template <typename T>
void write(std::ostream& os, T data)
{
    write(os, &data, 1);
}

template <typename T>
typename std::enable_if<std::is_trivially_copyable<T>::value>::type
read(std::istream& is, std::vector<T>* data)
{
    size_t size;
    read(is, &size);
    data->resize(size);
    read(is, &(*data)[0], size);
}

template <typename T>
void write(std::ostream& os, const std::vector<T>& data)
{
    write(os, data.size());
    for (const auto& elem : data) {
        write(os, elem);
    }
}

}}
