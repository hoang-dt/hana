#pragma once

#include "../core/types.h"
#include "../core/vector.h"
#include <cstdint>

namespace hana { namespace idx {

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
        };
        return 0;
    }
};

/** A 3D extend. */
struct Volume {
    core::Vector3i from;
    core::Vector3i to;

    bool is_valid() const;
    bool is_inside(const Volume& other) const;
    uint64_t get_num_samples() const;
};


/** Rectilinear grid in 3D. */
struct Grid {
    Volume extent; // [from, to] in x, y, z
    core::MemBlockChar data;
    IdxType type;
};

}}
