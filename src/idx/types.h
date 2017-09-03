#pragma once

#include "types.h"
#include "vector.h"
#include <cstdint>
#include <iosfwd>

namespace hana {

/** Encapsulate a pointer and a size so that we don't have to pass them to
functions separately. */
template <typename T>
struct ArrayRef {
	T* ptr = nullptr;
	size_t size = 0;
	ArrayRef() : ptr(nullptr), size(0) {}
	ArrayRef(T* p, size_t s) : ptr(p), size(s) {}
	T& operator[](size_t i) { HANA_ASSERT(i < size); return ptr[i]; }
	const T& operator[](size_t i) const { HANA_ASSERT(i < size); return ptr[i]; }
};

using BufferRef = ArrayRef<char>;

// A memory block with a pointer to the beginning of the block and a size in bytes
template <typename T>
struct MemBlock {
	T* ptr = nullptr;
	size_t bytes = 0;
	MemBlock() = default;
	MemBlock(T* p, size_t b) : ptr(p), bytes(b) {}

	template <typename T2>
	MemBlock(const MemBlock<T2>& b2)
		: ptr(static_cast<T*>(b2.ptr))
		, bytes(b2.bytes) {}

	template <typename T2>
	MemBlock& operator=(const MemBlock<T2>& b2)
	{
		ptr = static_cast<T*>(b2.ptr);
		bytes = b2.bytes;
		return *this;
	}
};

using MemBlockVoid = MemBlock<void>;
using MemBlockChar = MemBlock<char>;

enum class IdxPrimitiveType {
	Invalid,
	UInt8,
	UInt16,
	UInt32,
	UInt64,
	Int8,
	Int16,
	Int32,
	Int64,
	Float32,
	Float64
};

/**
The type of an IDX field, e.g. float64[3]. We only support primitive types (int,
float, double, etc) and arrays of primitive types. */
struct IdxType {
	IdxPrimitiveType primitive_type = IdxPrimitiveType::Invalid;
	int num_components = 0;
	int bytes() const {
		switch (primitive_type) {
			case IdxPrimitiveType::UInt8: return 1 * num_components;
			case IdxPrimitiveType::UInt16: return 2 * num_components;
			case IdxPrimitiveType::UInt32: return 4 * num_components;
			case IdxPrimitiveType::UInt64: return 8 * num_components;
			case IdxPrimitiveType::Int8: return 1 * num_components;
			case IdxPrimitiveType::Int16: return 2 * num_components;
			case IdxPrimitiveType::Int32: return 4 * num_components;
			case IdxPrimitiveType::Int64: return 8 * num_components;
			case IdxPrimitiveType::Float32: return 4 * num_components;
			case IdxPrimitiveType::Float64: return 8 * num_components;
			case IdxPrimitiveType::Invalid: return 0;
		};
		return 0;
	}
};

/** A 3D extend. */
struct Volume {
	Vector3i from;
	Vector3i to;

	bool is_valid() const;
	bool is_inside(const Volume& other) const;
	uint64_t get_num_samples() const;
};


/** Rectilinear grid in 3D. */
struct Grid {
	Volume extent; // [from, to] in x, y, z
	MemBlockChar data;
	IdxType type;
};

}
