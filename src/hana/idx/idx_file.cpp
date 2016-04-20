#include "../core/macros.h"
#include "../core/assert.h"
#include "../core/constants.h"
#include "../core/math.h"
#include "../core/string.h"
#include "../core/utils.h"
#include "idx_file.h"
#include "types.h"
#include "utils.h"
#include <catch.hpp>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <utility>
#include "catch.hpp"

namespace hana { namespace idx {

namespace {
    const char version_[] = "(version)";
    const char logic_to_physic_[] = "(logic_to_physic)";
    const char box_[] = "(box)";
    const char fields_[] = "(fields)";
    const char bits_[] = "(bits)";
    const char bitsperblock_[] = "(bitsperblock)";
    const char blocksperfile_[] = "(blocksperfile)";
    const char interleave_block_[] = "(interleave block)";
    const char time_[] = "(time)";
    const char filename_template_[] = "(filename_template)";
    const char default_compression_[] = "default_compression";
    const char compressed_[] = "compressed";
    const char default_layout_[] = "default_layout";
    const char filter_[] = "filter";
    const char format_[] = "format";
    const char min_[] = "min";
    const char max_[] = "max";
    const char description_[] = "description";
}

void IdxField::set_name(const char* s)
{
    strncpy(name, s, sizeof(name));
}

IdxFile::IdxFile(const core::Path& pth)
{
    absolute_path = pth;
    bits[0] = '\0';
}

IdxFile::IdxFile(const IdxFile& other)
    : IdxFileBase(other)
    , bit_string(core::StringRef(bits + 1)) {}

IdxFile& IdxFile::operator=(const IdxFile& other)
{
    detail::IdxFileBase::operator=(other);
    bit_string = core::StringRef(bits + 1);
    return *this;
}

namespace {

/** If the input is str = "abcd(efg)hik", open = '(', close = ')', this function
returns (abcd, efg). The rest of the string is ignored.
Either the first part or the second part of the returned pair can be empty. */
std::pair<core::StringRef, core::StringRef>
split_string(core::StringRef str, char open, char close)
{
    using namespace hana::core;
    StringRef first, second;
    size_t open_pos = find(str, StringRef(&open, 1));
    size_t close_pos = find(str, StringRef(&close, 1));
    if (open_pos != size_max_) {
        first = sub_string(str, 0, open_pos);
    }
    if (open_pos < close_pos && close_pos < size_max_) {
        second = sub_string(str, open_pos + 1, close_pos - open_pos - 1);
    }
    return std::make_pair(first, second);
}

/** If the input is str = "abcd*efg", split = '*', this function returns (abcd, efg).
Either the first part or the second part of the returned pair can be empty. */
std::pair<core::StringRef, core::StringRef>
split_string(core::StringRef str, char split)
{
    using namespace hana::core;
    StringRef first, second;
    size_t pos = find(str, StringRef(&split, 1));
    if (pos != size_max_) {
        first = sub_string(str, 0, pos);
        second = sub_string(str, pos + 1);
    }
    return std::make_pair(first, second);
}

IdxPrimitiveType string_to_primitive_type(core::StringRef str)
{
    if (str == STR_REF("uint8")  ) { return IdxPrimitiveType::UInt8;   }
    if (str == STR_REF("uint16") ) { return IdxPrimitiveType::UInt16;  }
    if (str == STR_REF("uint32") ) { return IdxPrimitiveType::UInt32;  }
    if (str == STR_REF("uint64") ) { return IdxPrimitiveType::UInt64;  }
    if (str == STR_REF("int8")   ) { return IdxPrimitiveType::Int8;    }
    if (str == STR_REF("int16")  ) { return IdxPrimitiveType::Int16;   }
    if (str == STR_REF("int32")  ) { return IdxPrimitiveType::Int32;   }
    if (str == STR_REF("int64")  ) { return IdxPrimitiveType::Int64;   }
    if (str == STR_REF("float32")) { return IdxPrimitiveType::Float32; }
    if (str == STR_REF("float64")) { return IdxPrimitiveType::Float64; }
    return IdxPrimitiveType::Invalid;
}

IdxType string_to_type(core::StringRef str)
{
    using namespace hana::core;
    IdxType type;
    size_t beg_bracket = find(str, STR_REF("["));
    if (beg_bracket == size_max_) {
        size_t asterisk_pos = find(str, STR_REF("*"));
        if (asterisk_pos == size_max_) { // the field is not a vector
            type.primitive_type = string_to_primitive_type(str);
            type.num_components = 1;
        }
        else { // the field is a vector (e.g. 3*float64)
            auto str_pair = split_string(str, '*');
            type.primitive_type = string_to_primitive_type(str_pair.second);
            to_int(str_pair.first, type.num_components);
        }
    }
    else { // the field is a vector (e.g. float64[3])
        auto str_pair = split_string(str, '[', ']');
        type.primitive_type = string_to_primitive_type(str_pair.first);
        to_int(str_pair.second, type.num_components);
    }
    return type;
}

Compression str_to_compression(core::StringRef str)
{
    using namespace hana::core;
    if (str == STR_REF("zip"))  { return Compression::Zip;  }
    if (str == STR_REF("jpeg")) { return Compression::Jpg; }
    if (str == STR_REF("exr"))  { return Compression::Exr;  }
    if (str == STR_REF("png"))  { return Compression::Png;  }
    if (str == STR_REF("zfp"))  { return Compression::Zfp;  }
    return Compression::None;
}

bool verify_idx_field(const IdxField& field)
{
    if (field.type.primitive_type == IdxPrimitiveType::Invalid) {
        return false;
    }
    if (field.type.num_components == 0) {
        return false;
    }
    if (field.name[0] == '\0') {
        return false;
    }
    return true;
}

/** Break a path string that represents a file name template
(e.g. ./%02x/%01x/%01x.bin) into its individual components. */
void parse_file_name_template(const core::Path& path, FileNameTemplate* ftmp)
{
    using namespace hana::core;
    HANA_ASSERT(ftmp);
    size_t dot = find_last(path.last(), STR_REF("."));
    copy(STR_REF(ftmp->ext), sub_string(path.last(), dot));
    size_t i = 0;
    for (const auto& component : path) {
        if (component.size >= 4 && start_with(component, STR_REF("%"))) { // e.g. %03x
            ftmp->num_hex_bits[i++] = component[2] - '0';
        }
        else {
            ftmp->head.add_component(component);
        }
    }
    // reverse the non-zero num hex bits (e.g. [3, 1, 2, 0, 0] -> [2, 1, 3, 0, 0])
    // we need to do this to facilitate constructing the full path later
    if (i > 0) {
        reverse(ArrayRef<int>(ftmp->num_hex_bits, i));
    }
}

idx::Error read_idx_file(std::istream& input, IdxFile* idx_file)
{
    using namespace hana::core;

    char line[1024] = "";
    bool parsing_fields = false;
    idx_file->num_fields = 0;
    while (input) {
        input.getline(line, sizeof(line));
        if (parsing_fields && line[0] == '(') {
            parsing_fields = false;
        }
        if (STRING_EQUAL(line, version_)) { // version
            input >> idx_file->version;
            input.ignore();
        }
        else if (STRING_EQUAL(line, logic_to_physic_)) { // logic_to_physic
            for (size_t i = 0; i < ARRAY_SIZE(idx_file->logic_to_physic); ++i) {
                input >> idx_file->logic_to_physic[i];
            }
            input.ignore();
        }
        else if (STRING_EQUAL(line, box_)) { // box
            input >> idx_file->box.from.x >> idx_file->box.to.x;
            input >> idx_file->box.from.y >> idx_file->box.to.y;
            input >> idx_file->box.from.z >> idx_file->box.to.z;
            int temp;
            input >> temp >> temp >> temp >> temp; // who cares about 4th and 5th dimensions?
            input.ignore();
        }
        else if (STRING_EQUAL(line, bits_)) { // bits
            input >> idx_file->bits;
            idx_file->bit_string = StringRef(idx_file->bits + 1);
            input.ignore();
        }
        else if (STRING_EQUAL(line, bitsperblock_)) { // bitsperblock
            input >> idx_file->bits_per_block;
            input.ignore();
        }
        else if (STRING_EQUAL(line, blocksperfile_)) { // blocksperfile
            input >> idx_file->blocks_per_file;
            input.ignore();
        }
        else if (STRING_EQUAL(line, interleave_block_)) { // interleave_block
            input >> idx_file->interleave_block;
            input.ignore();
        }
        else if (STRING_EQUAL(line, time_)) { // time
            input >> idx_file->time.begin >> idx_file->time.end;
            input >> idx_file->time.template_;
            input.ignore();
        }
        else if (STRING_EQUAL(line, filename_template_)) { // filename_template
            input.getline(line, sizeof(line));
            if (line[0] == '.' && line[1] == '/') {
                parse_file_name_template(Path(StringRef(line + 2)), &idx_file->filename_template);
            }
            else {
                parse_file_name_template(Path(StringRef(line)), &idx_file->filename_template);
            }
        }
        else if (STRING_EQUAL(line, fields_)) { // fields
            parsing_fields = true;
        }
        else if (parsing_fields && line[0] != '(') {
            IdxField& field = idx_file->fields[idx_file->num_fields++];
            StringTokenizer tokenizer(line);
            StringRef token = tokenizer.next();
            if (start_with(token, STR_REF("+"))) {
                token = tokenizer.next();
            }

            copy(STR_REF(field.name), token); // name

            token = tokenizer.next();
            field.type = string_to_type(token); // type

            token = tokenizer.next();
            while (token) {
                if (start_with(token, STR_REF(default_compression_))) { // default_compression
                    auto str_pair = split_string(token, '(', ')');
                    field.compression = str_to_compression(str_pair.second);
                }
                else if (start_with(token, STR_REF(compressed_))) { // old format
                    auto str_pair = split_string(token, '(', ')');
                    if (str_pair.second) { // compression specified
                        field.compression = str_to_compression(str_pair.second);
                    }
                    else { // no default compression specified, default to zip
                        field.compression = Compression::Zip;
                    }
                }
                else if (start_with(token, STR_REF(default_layout_)) ||
                         start_with(token, STR_REF(format_))) { // format
                    auto str_pair = split_string(token, '(', ')');
                    int fmt = 0;
                    to_int(str_pair.second, fmt);
                    field.format = static_cast<Format>(fmt);
                }
                else if (start_with(token, STR_REF(filter_))) { // filter
                    // TODO
                }
                else if (start_with(token, STR_REF(min_))) {
                    // TODO
                }
                else if (start_with(token, STR_REF(max_))) {
                    // TODO
                }
                else if (start_with(token, STR_REF(description_))) {
                    // TODO
                }
                else {
                    set_error_msg(fields_);
                    return Error::ParsingError;
                }
                token = tokenizer.next();
            }
        }
        else if (line[0] != '\0') {
            set_error_msg(line);
            return Error::ParsingError;
        }
    }

    if (!verify_idx_file(*idx_file)) {
        return Error::ParsingError;
    }

    return Error::NoError;
}

} // namespace

int IdxFile::get_num_fields() const
{
    return num_fields;
}

uint64_t IdxFile::get_size(const Volume& sub_vol, int field) const
{
    HANA_ASSERT(field >= 0 && field < num_fields);
    HANA_ASSERT(sub_vol.is_inside(box));
    int bytes_per_sample = fields[field].type.bytes();
    return sub_vol.get_num_samples() * bytes_per_sample;
}

uint64_t IdxFile::get_size(const Volume& sub_vol, int field, int hz_level) const
{
    HANA_ASSERT(field >= 0 && field < num_fields);
    HANA_ASSERT(hz_level >= 0 && hz_level <= get_max_hz_level());
    HANA_ASSERT(sub_vol.is_inside(box));
    core::Vector3i from, to, stride;
    get_grid(sub_vol, hz_level, from, to, stride);
    core::Vector3u64 dims = (to - from) / stride + 1;
    return dims.x * dims.y * dims.z * fields[field].type.bytes();
}

uint64_t IdxFile::get_size_inclusive(const Volume& sub_vol, int field, int hz_level) const
{
    HANA_ASSERT(field >= 0 && field < num_fields);
    HANA_ASSERT(hz_level >= 0 && hz_level <= get_max_hz_level());
    HANA_ASSERT(sub_vol.is_inside(box));
    core::Vector3i from, to, stride;
    get_grid_inclusive(sub_vol, hz_level, from, to, stride);
    core::Vector3u64 dims = (to - from) / stride + 1;
    return dims.x * dims.y * dims.z * fields[field].type.bytes();
}

uint64_t IdxFile::get_size(int field) const
{
    return get_size(box, field);
}

uint64_t IdxFile::get_size(int field, int hz_level) const
{
    return get_size(box, field, hz_level);
}

uint64_t IdxFile::get_size_inclusive(int field, int hz_level) const
{
    return get_size_inclusive(box, field, hz_level);
}

bool IdxFile::get_grid(int hz_level, OUT core::Vector3i& from,
                       OUT core::Vector3i& to, OUT core::Vector3i& stride) const
{
    HANA_ASSERT(hz_level >= 0 && hz_level <= get_max_hz_level());
    return get_grid(box, hz_level, from, to, stride);
}

bool IdxFile::get_grid(const Volume& sub_vol, int hz_level,
    OUT core::Vector3i& from, OUT core::Vector3i& to, OUT core::Vector3i& stride) const
{
    HANA_ASSERT(sub_vol.is_valid() && sub_vol.is_inside(box));
    HANA_ASSERT(hz_level >= 0 && hz_level <= get_max_hz_level());
    stride = get_intra_block_strides(bit_string, hz_level);
    core::Vector3i start = get_first_coord(bit_string, hz_level);
    core::Vector3i end = get_last_coord(bit_string, hz_level);
    return intersect_grid(sub_vol, start, end, stride, from, to);
}

bool IdxFile::get_grid_inclusive(const Volume& sub_vol, int hz_level,
    OUT core::Vector3i& from, OUT core::Vector3i& to, OUT core::Vector3i& stride) const
{
    HANA_ASSERT(sub_vol.is_valid() && sub_vol.is_inside(box));
    HANA_ASSERT(hz_level >= 0 && hz_level <= get_max_hz_level());
    core::Vector3i start = core::Vector3i(0, 0, 0);
    core::Vector3i end = get_last_coord(bit_string, hz_level);
    stride = get_intra_block_strides(bit_string, hz_level + 1);
    return intersect_grid(sub_vol, start, end, stride, from, to);
}

bool IdxFile::get_grid_inclusive(int hz_level, OUT core::Vector3i& from,
    OUT core::Vector3i& to, OUT core::Vector3i& stride) const
{
    return get_grid_inclusive(box, hz_level, from, to, stride);
}

core::Vector3i IdxFile::get_dims(int hz_level) const
{
    return get_dims(box, hz_level);
}

core::Vector3i IdxFile::get_dims(const Volume& sub_vol, int hz_level) const
{
    core::Vector3i from, to, stride;
    get_grid(sub_vol, hz_level, from, to, stride);
    return (to - from) / stride + 1;
}

core::Vector3i IdxFile::get_dims_inclusive(int hz_level) const
{
    return get_dims_inclusive(box, hz_level);
}

core::Vector3i IdxFile::get_dims_inclusive(const Volume& sub_vol, int hz_level) const
{
    core::Vector3i from, to, stride;
    get_grid_inclusive(sub_vol, hz_level, from, to, stride);
    return (to - from) / stride + 1;
}

uint64_t IdxFile::get_logical_size() const
{
    return get_logical_size_per_time_step() * get_num_time_steps();
}

uint64_t IdxFile::get_logical_size_per_time_step() const
{
    uint64_t size = 0;
    for (int i = 0; i < num_fields; ++i) {
        size += get_logical_size_per_time_step_field(i);
    }
    return size;
}

uint64_t IdxFile::get_logical_size_per_time_step_field(int field) const
{
    return get_size(box, field);
}

int IdxFile::get_max_hz_level() const
{
    return static_cast<int>(bit_string.size);
}

int IdxFile::get_min_hz_level() const
{
    return bits_per_block + 1;
}

uint64_t IdxFile::get_num_samples_per_field() const
{
    return get_logical_extent().get_num_samples();
}

uint64_t IdxFile::get_num_samples_per_block() const
{
    uint64_t num_samples = 1;
    for (int i = 0; i < bits_per_block; ++i) {
        num_samples *= 2;
    }
    return num_samples;
}

int IdxFile::get_field_sample_size(int field) const
{
    if (field < 0 || field >= num_fields) {
        return 0;
    }
    return fields[field].type.bytes();
}

Volume IdxFile::get_logical_extent() const
{
    return box;
}

int IdxFile::get_field_index(const char* name) const
{
    core::StringRef name_ref(name);
    for (int i = 0; i < num_fields; ++i) {
        if (core::StringRef(fields[i].name) == name_ref) {
            return i;
        }
    }
    return -1;
}

int IdxFile::get_min_time_step() const
{
    return time.begin;
}

int IdxFile::get_max_time_step() const
{
    return time.end;
}

int IdxFile::get_num_time_steps() const
{
    return time.end - time.begin + 1;
}

bool verify_idx_file(const IdxFile& idx_file)
{
    using namespace hana::core;

    if (idx_file.version <= 0) {
        set_error_msg(version_);
        return false;
    }

    if (idx_file.box.from.x < 0 || idx_file.box.from.y < 0 || idx_file.box.from.z < 0 ||
        idx_file.box.to.x < 0 || idx_file.box.to.y < 0 || idx_file.box.to.z < 0 ||
        idx_file.box.from.x > idx_file.box.to.x ||
        idx_file.box.from.y > idx_file.box.to.y ||
        idx_file.box.from.z > idx_file.box.to.z) {
        return false;
    }

    if (idx_file.num_fields <= 0) {
        return false;
    }
    for (int i = 0; i < idx_file.num_fields; ++i) {
        if (!verify_idx_field(idx_file.fields[i])) {
            set_error_msg(fields_);
            return false;
        }
    }

    if (idx_file.bits[0] != 'V') {
        set_error_msg(bits_);
        return false;
    }

    if (idx_file.bits_per_block <= 0) {
        set_error_msg(bitsperblock_);
        return false;
    }

    if (idx_file.blocks_per_file <= 0) {
        set_error_msg(blocksperfile_);
        return false;
    }

    if (idx_file.filename_template.head.num_components() == 0 ||
        idx_file.filename_template.num_hex_bits[0] == 0 ||
        !idx_file.filename_template.ext) {
        set_error_msg(filename_template_);
        return false;
    }

    return true;
}

idx::Error read_idx_file(const char* file_path, OUT IdxFile* idx_file)
{
    HANA_ASSERT(file_path);
    HANA_ASSERT(idx_file);

    // if the given file name is relative, we get the current directory and add
    // it to the beginning of the given file name
    char buffer[512];
    core::StringRef file_path_ref(file_path);
    if (is_relative_path(file_path_ref)) {
        core::get_current_dir(STR_REF(buffer));
        core::StringRef current_path(buffer);
        core::replace(current_path, '\\', '/');
        idx_file->absolute_path.construct_from(current_path);
        idx_file->absolute_path.append(core::Path(file_path_ref));
    }
    else {
        idx_file->absolute_path.construct_from(file_path_ref);
    }
    idx_file->absolute_path.remove_last(); // remove the file name
    std::ifstream input(file_path_ref.ptr);
    if (!input) {
        return core::Error::FileNotFound;
    }
    return read_idx_file(input, idx_file);
}

/** Create an IDX file given the dimensions, number of fields, type of every field,
and the number of time steps.
Note that the exact type of each field can be changed later if needed. The same
goes for the exact name of each field. By default, the fields are named data0,
data1, etc. */
void create_idx_file(const core::Vector3i& dims, int num_fields,
                     const IdxType& type, int num_time_steps, OUT IdxFile* idx_file)
{
    using namespace core;

    HANA_ASSERT(dims.x > 0 && dims.y > 0 && dims.z > 0);
    HANA_ASSERT(num_fields > 0 && num_fields <= IdxFile::num_fields_max);
    HANA_ASSERT(num_time_steps > 0);
    HANA_ASSERT(idx_file != nullptr);

    idx_file->box.from = Vector3i(0, 0, 0);
    idx_file->box.to = dims - 1;
    idx_file->num_fields = num_fields;
    for (int i = 0; i < num_fields; ++i) {
        IdxField& field = idx_file->fields[i];
        char temp[16];
        sprintf(temp, "data%d", i);
        field.set_name(temp);
        field.type = type;
        field.format = Format::RowMajor;
        field.compression = Compression::None;
    }

    idx_file->bit_string = StringRef(idx_file->bits);
    guess_bit_string(dims, idx_file->bit_string);
    int pow2_x = pow_greater_equal(2, dims.x);
    int pow2_y = pow_greater_equal(2, dims.y);
    int pow2_z = pow_greater_equal(2, dims.z);
    uint64_t total_samples = (uint64_t)pow2_x * pow2_y * pow2_z;


    idx_file->blocks_per_file = 256;

    // TODO
}

}}
