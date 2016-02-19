/**\file
This file contains data structure that represents an IDX file.
*/

#pragma once

#include "../core/macros.h"
#include "../core/error.h"
#include "../core/filesystem.h"
#include "../core/string.h"
#include "error.h"
#include "types.h"

/**\namespace hana::idx */
namespace hana { namespace idx {

enum class Compression {
    Invalid,
    None,
    Zip,
    Jpg,
    Exr,
    Png,
    Zfp
};

enum Format {
    RowMajor,
    Hz
};

/** An IDX field */
struct IdxField {
    IdxType type = {};
    char name[128] = "";
    Format format = Format::RowMajor; // TODO: what is the default?
    Compression compression = Compression::None;
};

/** Range and format of time steps */
struct IdxTime {
    int begin = 0;
    int end = 0;
    /** Template, e.g. time%06d */
    char template_[64] = "";
};

/** Store the different components of a file name template.
A file name template has the form ./%02x/%01x/%01x.bin */
struct FileNameTemplate {
    core::Path head; /** This part is fixed (e.g. .) */
    int num_hex_bits[64] = {}; /** e.g. [2, 1, 1] */
    char ext[8] = ""; /** e.g. .bin */
};

/** An IDX file */
struct IdxFile {
    /** Absolute path to the idx file itself. */
    core::Path absolute_path;
    int version = 0;
    /** 4x4 matrix that transforms the data from logical space to physical space */
    float logic_to_physic[16] = {}; // TODO: row major or column major?
    /** Logical extends of the dataset (5 pairs of begin-end, inclusive at both ends) */
    Volume box;
    IdxField fields[512] = {};
    int num_fields = 0;
    /** Bit string (e.g. V012012012) */
    char bits[64];
    /** Refers to the "bits" buffer above, but without the V and only until the NULL character. */
    core::StringRef bit_string;
    /** 2^bits_per_block = number of samples per IDX block */
    int bits_per_block = 0;
    /** Number of idx blocks in a binary file. */
    int blocks_per_file = 0;
    int interleave_block = 0; // TODO: what is this
    IdxTime time;
    FileNameTemplate filename_template;

    IdxFile() = default;
    IdxFile(const core::Path& pth) : absolute_path(pth) { bits[0] = '\0'; }

    /** Get the maximum HZ level (the same as the length of the bit string).
    NOTE: the maximum HZ level does not represent the whole data set (only half
    of it). */
    int get_max_hz_level() const;
    /** Get the minimum HZ level (which is not 0, but the HZ level where the
    number of samples is equal to that of an IDX block). All the HZ levels below
    this one will belong to the first block. */
    int get_min_hz_level() const;

    /** Given a field's name, return its index. Return -1 if not found. */
    int get_field_index(const char* name) const;

    int get_min_time_step() const;
    int get_max_time_step() const;
    int get_num_time_steps() const;

    /** e.g. 512x256x256 */
    Volume get_logical_extent() const;

    /** Return the number of fields. */
    int get_num_fields() const;

    /** Get the size (in bytes) of the whole dataset assuming it is uncompressed. */
    uint64_t get_uncompressed_size() const; // TODO
    uint64_t get_uncompressed_size_per_time_step() const;  // TODO

    /** The logical size (in bytes) is the size of the dataset assuming it was
    stored in raw binary format (i.e. no overhead). */
    uint64_t get_logical_size() const;
    uint64_t get_logical_size_per_time_step() const;
    uint64_t get_logical_size_per_time_step_field(int field) const;

    /** Given a sub-volume, return the size (in bytes) that would be needed to
    store all the samples (non-compressed) of this sub-volume in a grid. */
    uint64_t get_size(const Volume& sub_vol, int field) const;
    uint64_t get_size(const Volume& sub_vol, int field, int hz_level) const;
    uint64_t get_size_inclusive(const Volume& sub_vol, int field, int hz_level) const;
    uint64_t get_size(int field) const;
    uint64_t get_size(int field, int hz_level) const;
    uint64_t get_size_inclusive(int field, int hz_level) const;

    /** Return the total (per field) number of IDX blocks. */
    uint64_t get_num_blocks_per_field() const; // TODO
    /** Return the (per field) number of IDX blocks at a given hz level. */
    uint64_t get_num_blocks_per_field(int hz_level) const; // TODO

    /** Given a field, return the size (in bytes, non-compressed) of a sample in
    this field. */
    int get_field_sample_size(int field) const;

    /** Return the total number of samples per field. */
    uint64_t get_num_samples_per_field() const;
    uint64_t get_num_samples_per_field(int hz_level) const; // TODO
    uint64_t get_num_samples_per_block() const;

    /** Given an hz level, compute the grid corresponding to this hz level. */
    bool get_grid(int hz_level, OUT core::Vector3i& from, OUT core::Vector3i& to,
                  OUT core::Vector3i& stride) const;
    /** Given an hz level and a sub-volume, compute the grid in this hz level that
    intersects with the sub-volume. Return false if the resulting grid has no
    samples. */
    bool get_grid(const Volume& sub_vol, int hz_level, OUT core::Vector3i& from,
                  OUT core::Vector3i& to, OUT core::Vector3i& stride) const;
    bool get_grid_inclusive(int hz_level, OUT core::Vector3i& from,
                            OUT core::Vector3i& to, OUT core::Vector3i& stride) const;
    bool get_grid_inclusive(const Volume& sub_vol, int hz_level,
                            OUT core::Vector3i& from, OUT core::Vector3i& to,
                            OUT core::Vector3i& stride) const;

    core::Vector3i get_dims(int hz_level) const;
    core::Vector3i get_dims(const Volume& sub_vol, int hz_level) const;
    core::Vector3i get_dims_inclusive(int hz_level) const;
    core::Vector3i get_dims_inclusive(const Volume& sub_vol, int hz_level) const;
};

/** Return true if the idx file is valid. */
bool verify_idx_file(const IdxFile& idx_file);

/** Read an IDX (text) file into memory. */
idx::Error read_idx_file(const char* file_path, OUT IdxFile* idx_file);


}}
