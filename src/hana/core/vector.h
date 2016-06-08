#pragma once

#include "assert.h"
#include <cstdint>
#include <iosfwd>

namespace hana { namespace core {

/** 2D vector */
template <typename T>
struct Vector2 {
    T x = 0;
    T y = 0;

    Vector2() = default;
    Vector2(T x_, T y_) : x(x_), y(y_) {}
    template <typename U>
    Vector2(const Vector2<U>& other) : x(other.x), y(other.y) {}
    template <typename U>
    Vector2& operator=(const Vector2<U>& other) { x = other.x; y = other.y; }
};

template <typename T>
bool operator<=(const Vector2<T>& a, const Vector2<T>& b)
{
    return a.x <= b.x && a.y <= b.y;
}

template <typename T>
bool operator==(const Vector2<T>& a, const Vector2<T>& b)
{
    return a.x == b.x && a.y == b.y;
}

template <typename T>
Vector2<T> operator+(const Vector2<T>& a, T x)
{
    return Vector2<T>(a.x + x, a.y + x);
}

template <typename T>
Vector2<T> operator+(const Vector2<T>& a, const Vector2<T>& b)
{
    return Vector2<T>(a.x + b.x, a.y + b.y);
}

template <typename T>
Vector2<T> operator-(const Vector2<T>& a, T x)
{
    return Vector2<T>(a.x - x, a.y - x);
}

template <typename T>
Vector2<T> operator-(const Vector2<T>& a, const Vector2<T>& b)
{
    return Vector2<T>(a.x - b.x, a.y - b.y);
}

template <typename T>
Vector2<T> operator*(const Vector2<T>& a, const Vector2<T>& b)
{
    return Vector2<T>(a.x * b.x, a.y * b.y);
}

template <typename T>
Vector2<T> operator*(const Vector2<T>& a, T x)
{
    return Vector2<T>(a.x * x, a.y * x);
}

template <typename T>
Vector2<T> operator/(const Vector2<T>& a, const Vector2<T>& b)
{
    T x = b.x != 0 ? a.x / b.x : 0;
    T y = b.y != 0 ? a.y / b.y : 0;
    return Vector2<T>(x, y);
}

using Vector2u64 = Vector2<uint64_t>;
using Vector2i = Vector2<int>;

template <typename T>
std::ostream& operator<<(std::ostream& os, const Vector2<T>& v)
{
    os << v.x << " " << v.y;
    return os;
}

template <typename T>
std::istream& operator >> (std::istream& os, Vector2<T>& v)
{
    os >> v.x >> v.y;
    return os;
}

/** 3D vector */
template <typename T>
struct Vector3 {
    T x = 0;
    T y = 0;
    T z = 0;

    Vector3() = default;
    Vector3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}
    template <typename U>
    Vector3(const Vector3<U>& other) : x(other.x), y(other.y), z(other.z) {}
    template <typename U>
    Vector3& operator=(const Vector3<U>& other) { x = other.x; y = other.y; z = other.z; }
};

template <typename T>
bool operator<=(const Vector3<T>& a, const Vector3<T>& b)
{
    return a.x <= b.x && a.y <= b.y && a.z <= b.z;
}

template <typename T>
bool operator==(const Vector3<T>& a, const Vector3<T>& b)
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

template <typename T>
Vector3<T> operator+(const Vector3<T>& a, T x)
{
    return Vector3<T>(a.x + x, a.y + x, a.z + x);
}

template <typename T>
Vector3<T> operator+(const Vector3<T>& a, const Vector3<T>& b)
{
    return Vector3<T>(a.x + b.x, a.y + b.y, a.z + b.z);
}

template <typename T>
Vector3<T> operator-(const Vector3<T>& a, T x)
{
    return Vector3<T>(a.x - x, a.y - x, a.z - x);
}

template <typename T>
Vector3<T> operator-(const Vector3<T>& a, const Vector3<T>& b)
{
    return Vector3<T>(a.x - b.x, a.y - b.y, a.z - b.z);
}

template <typename T>
Vector3<T> operator*(const Vector3<T>& a, const Vector3<T>& b)
{
    return Vector3<T>(a.x * b.x, a.y * b.y, a.z * b.z);
}

template <typename T>
Vector3<T> operator*(const Vector3<T>& a, T x)
{
    return Vector3<T>(a.x * x, a.y * x, a.z * x);
}

template <typename T>
Vector3<T> operator/(const Vector3<T>& a, const Vector3<T>& b)
{
    T x = b.x != 0 ? a.x / b.x : 0;
    T y = b.y != 0 ? a.y / b.y : 0;
    T z = b.z != 0 ? a.z / b.z : 0;
    return Vector3<T>(x, y, z);
}

using Vector3u64 = Vector3<uint64_t>;
using Vector3i = Vector3<int>;

template <typename T>
std::ostream& operator<<(std::ostream& os, const Vector3<T>& v)
{
    os << v.x << " " << v.y << " " << v.z;
    return os;
}

template <typename T>
std::istream& operator>>(std::istream& os, Vector3<T>& v)
{
    os >> v.x >> v.y >> v.z;
    return os;
}

}}
