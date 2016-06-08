/** \file
This file defines various memory allocators.
*/

#pragma once

#include "assert.h"
#include "types.h"
#include <cstdlib>

/** \namespace hana::memory */
namespace hana { namespace core {

//------------------------------------------------
// Allocator
//------------------------------------------------

/** Interface for all allocators */
class Allocator {
  public:
    /** Allocate memory and return the allocated block */
    virtual MemBlockVoid allocate(size_t bytes) = 0;

    /** Return false if fail to deallocate */
    virtual bool deallocate(MemBlockVoid b) = 0;

    /** Return true if the given Block belongs to us */
    virtual bool owns(MemBlockVoid) const = 0;
};

//------------------------------------------------
// Mallocator
//------------------------------------------------

/** The C's malloc allocator */
class Mallocator : public Allocator {
  public:
    MemBlockVoid allocate(size_t bytes) override
    {
        auto p = malloc(bytes);
        if (p != nullptr) {
            return MemBlockVoid{ p, bytes };
        }
        return MemBlockVoid();
    }

    bool deallocate(MemBlockVoid b) override
    {
        free(b.ptr);
        return true;
    }

    bool owns(MemBlockVoid) const override
    {
        //HANA_ASSERT_MSG(false, "Mallocator does not support 'owns'");
        return false;
    }
};

/** A simple allocator that allocates simply by bumping a counter. */
class LinearAllocator : public Allocator {
  private:
    MemBlockVoid block_;
    size_t current_size_ = 0;

  public:
    LinearAllocator() = default;

    LinearAllocator(MemBlockVoid b)
        : block_(b) {}

    MemBlockVoid allocate(size_t bytes) override
    {
        if (current_size_ + bytes <= block_.bytes) {
            MemBlockVoid b = MemBlockVoid{(char*)block_.ptr + current_size_, bytes};
            current_size_ += bytes;
            return b;
        }
        else {
            return MemBlockVoid();
        }
    }

    bool deallocate(MemBlockVoid) override { return false; }

    void deallocate_all() { current_size_ = 0; }

    bool owns(MemBlockVoid p) const override
    {
        return block_.ptr <= p.ptr && p.ptr <= (char*)block_.ptr + block_.bytes;
    }
};

/** A linear allocator that uses stack storage. */
template <size_t Capacity>
class InSituLinearAllocator : public LinearAllocator {
  private:
    char storage[Capacity];

  public:
    static const size_t capacity = Capacity;

    InSituLinearAllocator()
        : LinearAllocator(MemBlockVoid(storage, Capacity)) {}
};

// TODO: (double-ended) StackAllocator
// TODO: aligned allocation
// TODO: pool allocator

/** Whenever an allocation of a size in a specific range is made, return the
block immediately from the head of a linked list. Otherwise forward the allocation
to some Parent allocator. */
template <typename Parent>
class FreelistAllocator : public Allocator {
  private:
    /** The allocator that we forward to. */
    Parent parent_;
    /** Linked list structure to keep track of free blocks. */
    struct Node { Node* next = nullptr; };
    /** The head of the linked list which points to the first free block. */
    Node* head_ = nullptr;
    /** The minimum allocation size supported. */
    size_t min_size_ = 0;
    /** The maximum allocation size supported. */
    size_t max_size_ = 0;

  public:
    FreelistAllocator() = default;

    FreelistAllocator(size_t min_size, size_t max_size)
        : min_size_(min_size)
        , max_size_(max_size) {}

    MemBlockVoid allocate(size_t bytes) override
    {
        if (min_size_ <= bytes && bytes <= max_size_ && head_) {
            MemBlockVoid b { head_, max_size_ };
            head_ = head_->next;
            return b;
        }
        else {
            return parent_.allocate(bytes);
        }
    }

    bool deallocate(MemBlockVoid b) override
    {
        if (b.bytes == max_size_) {
            Node* p = static_cast<Node*>(b.ptr);
            p->next = head_;
            head_ = p;
            return true;
        }
        else {
            return parent_.deallocate(b);
        }
    }

    bool owns(MemBlockVoid b) const override
    {
        return (b.bytes >= min_size_ && b.bytes <= max_size_) || parent_.owns(b);
    }

    void deallocate_all()
    {
        while (head_) {
            Node* next = head_->next;
            parent_.deallocate(MemBlockVoid{ head_, max_size_ });
            head_ = next;
        }
    }

    size_t size() { return max_size_; }

    void set_sizes(size_t min_size, size_t max_size)
    {
        if (max_size != max_size_) {
            deallocate_all();
        }
        min_size_ = min_size;
        max_size_ = max_size;
    }
};

//------------------------------------------------
// FallbackAllocator
//------------------------------------------------

/** Try to allocate using one allocator first (the Primary), then if that fails,
use another allocator (the Secondary). */
template <typename Primary, typename Secondary>
class FallbackAllocator : public Allocator {
  private:
    // Default allocators if nothing is passed to us
    Primary _primary;
    Secondary _secondary;
    // Member functions use these two pointers instead of the two objects above
    Primary* _primary_ptr = &_primary;
    Secondary* _secondary_ptr = &_secondary;

  public:
    FallbackAllocator() = default;

    FallbackAllocator(Primary& primary, Secondary& secondary)
        : _primary_ptr(&primary)
        , _secondary_ptr(&secondary) {}

    MemBlockVoid allocate(size_t bytes) override
    {
        MemBlockVoid b = _primary_ptr->allocate(bytes);
        return b.ptr != nullptr ? b : _secondary_ptr->allocate(bytes);
    }

    bool deallocate(MemBlockVoid b) override
    {
        if (_primary_ptr->owns(b)) {
            return _primary_ptr->deallocate(b);
        }
        return _secondary_ptr->deallocate(b);
    }

    bool owns(MemBlockVoid b) const override
    {
        return _primary_ptr->owns(b) || _secondary_ptr->owns(b);
    }
};

}} // end namespace hana::core::memory
