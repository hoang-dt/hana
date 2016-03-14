#pragma once

#include <cstdint>

namespace hana { namespace core {

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

}}
