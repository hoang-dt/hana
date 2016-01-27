#pragma once

#include "allocator.h"
#include "assert.h"
#include <algorithm> // TODO: try to remove this
#include <cstring>
#include <type_traits>

namespace hana { namespace core {

/** Only works for POD types. For other types, use std::vector. */
template <typename T>
class Array {
    static_assert(std::is_trivially_copyable<T>::value, "Non-pod type argument");

  public:
    typedef T* iterator;
    typedef const T* const_iterator;

  private:
    size_t num_elems_ = 0;
    size_t num_elems_max_ = 0;
    T* buffer_ = nullptr;
    Allocator* alloc_ = nullptr;

  public:
    Array(Allocator* alloc)
        : alloc_(alloc)
    {
        HANA_ASSERT(alloc);
    }

    Array(Allocator* alloc, size_t num_elems)
        : alloc_(alloc)
        , num_elems_(num_elems)
        , num_elems_max_(num_elems_ * 3 / 2)
    {
        auto b = alloc_->allocate(num_elems_max_ * sizeof(T));
        buffer_ = static_cast<T*>(b.ptr);
    }

    Array(Allocator* alloc, size_t num_elems, const T& val)
        : Array(alloc, num_elems)
    {
        std::fill(buffer_, buffer_ + num_elems_, val);
    }

    Array(const Array& other)
        : num_elems_(other.num_elems_)
        , num_elems_max_(other.num_elems_max_)
        , alloc_(other.alloc_)
    {
        auto b = alloc_->allocate(num_elems_max_ * sizeof(T));
        memcpy(b.ptr, other.buffer_, other.num_elems_ * sizeof(T));
        buffer_ = b.ptr;
    }

    Array(Array&& other) noexcept
        : num_elems_(other.num_elems_)
        , num_elems_max_(other.num_elems_max_)
        , buffer_(other.buffer_)
        , alloc_(other.alloc_)
    {
        other.num_elems_ = 0;
        other.num_elems_max_ = 0;
        other.buffer_ = nullptr;
        other.alloc_ = nullptr;
    }

    Array& operator=(const Array& other)
    {
        auto b = other.alloc_->allocate(other.num_elems_max_ * sizeof(T));
        memcpy(b.ptr, other.buffer_, other.num_elems_ * sizeof(T));
        alloc_->deallocate(MemBlockVoid(buffer_, num_elems_max_));
        buffer_ = static_cast<T*>(b.ptr);
        num_elems_ = other.num_elems_;
        num_elems_max_ = other.num_elems_max_;
        return *this;
    }

    Array& operator=(Array&& other) noexcept
    {
        num_elems_ = other.num_elems_;
        num_elems_max_ = other.num_elems_max_;
        buffer_ = other.buffer_;
        alloc_ = other.alloc_;
        other.num_elems_ = 0;
        other.num_elems_max_ = 0;
        other.buffer_ = nullptr;
        other.alloc_ = nullptr;
        return *this;
    }

    const T& operator[](size_t i) const
    {
        HANA_ASSERT(i < num_elems_);
        return buffer_[i];
    }

    T& operator[](size_t i)
    {
        HANA_ASSERT(i < num_elems_);
        return buffer_[i];
    }

    const T& front() const
    {
        HANA_ASSERT(num_elems_ > 0);
        return buffer_[0];
    }

    T& front()
    {
        HANA_ASSERT(num_elems_ > 0);
        return buffer_[0];
    }

    const T& back() const
    {
        HANA_ASSERT(num_elems_ > 0);
        return buffer_[num_elems_ - 1];
    }

    T& back()
    {
        HANA_ASSERT(num_elems_ > 0);
        return buffer_[num_elems_ - 1];
    }

    iterator begin() { return buffer_; }
    iterator end() { return buffer_ + num_elems_; }
    const_iterator begin() const { return buffer_; }
    const_iterator end() const { return buffer_ + num_elems_; }

    size_t size() const { return num_elems_; }
    size_t capacity() const { return num_elems_max_; }

  private:
    void move_to_new_buffer(MemBlockVoid b)
    {
        memcpy(b.ptr, buffer_, num_elems_ * sizeof(T));
        alloc_->deallocate(MemBlockVoid(buffer_, num_elems_max_ * sizeof(T)));
        buffer_ = static_cast<T*>(b.ptr);
    }

  public:
    void push_back(const T& item)
    {
        if (num_elems_ >= num_elems_max_) {
            num_elems_max_ = num_elems_max_ * 3 / 2 + 8;
            move_to_new_buffer(alloc_->allocate(num_elems_max_ * sizeof(T)));
        }
        buffer_[num_elems_++] = item;
    }

    void clear()
    {
        num_elems_ = 0;
    }

    // TODO: insert
    // TODO: erase

    void resize(size_t num_elems_new)
    {
        if (num_elems_new <= num_elems_max_) {
            num_elems_ = num_elems_new;
        }
        else { // allocate new memory
            num_elems_max_ = num_elems_new * 3 / 2 + 8;
            move_to_new_buffer(alloc_->allocate(num_elems_max_ * sizeof(T)));
            num_elems_ = num_elems_new;
        }
    }

    void reserve(size_t new_capacity)
    {
        if (num_elems_max_ < new_capacity) {
            num_elems_max_ = new_capacity;
            move_to_new_buffer(alloc_->allocate(num_elems_max_ * sizeof(T)));
        }
    }

    void shrink_to_fit()
    {
        if (num_elems_ < num_elems_max_) {
            num_elems_max_ = num_elems_;
            move_to_new_buffer(alloc_->allocate(num_elems_max_ * sizeof(T)));
        }
    }

    void swap(Array& other)
    {
        std::swap(num_elems_, other.num_elems_);
        std::swap(num_elems_max_, other.num_elems_max_);
        std::swap(buffer_, other.buffer_);
        std::swap(alloc_, other.alloc_);
    }

    ~Array()
    {
        HANA_ASSERT(alloc_);
        alloc_->deallocate(MemBlockVoid(buffer_, num_elems_max_ * sizeof(T)));
    }
};

}} // end namespace hana::core
