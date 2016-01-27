#include "../core/logger.h"
#include "../core/array.h"
#include "../core/bitops.h"
#include "../core/streams.h"
#include "../core/string.h"
#include "../core/types.h"
#include "../core/utils.h"
#include "../idx/idx.h"
#include "../idx/idx_file.h"
#include <array>
#include <bitset>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <type_traits>
#include <functional>
#include <fstream>

void test_logger()
{
    using namespace hana::core::logger;
    enable_channel(0);
    log(LogLevel::Info, 0, "Info\n");
    log(LogLevel::Info, 0, "Info%d\n", 2);

    enable_channel(1);
    channel_name(1) = "Graphics";
    log(LogLevel::Warning, 1, "Warning\n");
    log_level() = LogLevel::Error;
    log(LogLevel::Warning, 1, "Muted Warning\n");

    enable_channel(2);
    if (open_channel_output(2, "logger_channel_2.txt")) {
        log(LogLevel::Fatal, 2, "Fatal %s\n", "error");
        close_channel_output(2);
    }

    enable_channel(3);
    if (open_channel_output(3, "logger_channel_3.txt")) {
        log(LogLevel::Error, 3, "Error\n");
        close_channel_output(3);
        disable_channel(3);
        log(LogLevel::Error, 3, "Error 2");
        enable_channel(3);
        channel_output(3) = stderr;
        log(LogLevel::Error, 3, "Error 3\n");
    }

    log_level() = LogLevel::Trace;
    log("The end\n");
}

void test_array()
{
    using namespace hana::core;
    Mallocator alloc;
    Array<int> arr(&alloc);
    arr.push_back(2);
    arr.push_back(3);
    std::cout << arr[0] << " " << arr[1] << "\n";
    arr.resize(4);
    arr[2] = 3;
    arr.back() = 4;
    arr.front() = 0;
    for (int i = 10; i < 30; ++i) {
        arr.push_back(i);
    }
    arr.resize(6);
    arr.shrink_to_fit();
    for (auto v : arr) {
        std::cout << v << " ";
    }
    std::cout << "size = " << arr.size() << ", capacity = " << arr.capacity() << "\n";

    Array<int> arr2(&alloc);
    arr2.push_back(100);
    arr2.push_back(101);
    arr = arr2;
    for (auto v : arr) {
        std::cout << v << " ";
    }
    std::cout << "\n";
    for (auto v : arr2) {
        std::cout << v << " ";
    }
}

void test_string_tokenizer()
{
    using namespace hana::core;
    StringTokenizer tokenizer("+ Flow-properties-Density float64 format(0)");
    std::cout << tokenizer.next().ptr << std::endl;
    std::cout << tokenizer.next().ptr << std::endl;
    std::cout << tokenizer.next().ptr << std::endl;
    std::cout << tokenizer.next().ptr << std::endl;
    HANA_ASSERT(tokenizer.next().ptr == nullptr);
}

void test_stream_buf()
{
    using namespace hana::core;
    InSituLinearAllocator<1024> alloc;
    OStreamBuf<char> stream_buf(&alloc, alloc.capacity);
    std::ostream out_stream(&stream_buf);
    out_stream << "Hello world " << std::endl;
    logger::enable_channel(0);
    logger::log("%s", stream_buf.buffer());
}

void test_string_ref()
{
    using namespace hana::core;
    StringRef a("hello world");
    StringRef b("hello");
    StringRef c("world");
    std::cout << sub_string(a, 6, 5) << "\n";
    std::cout << std::boolalpha << start_with(a, b) << "\n";
    std::cout << std::boolalpha << start_with(a, c) << "\n";
}

void test_idx_file()
{
    using namespace hana::idx;
    std::cout << sizeof(IdxFile) << "\n";
}

void test_string_to_int()
{
    using namespace hana::core;
    StringRef a("12345");
    StringRef b("-123");
    StringRef c("0");
    StringRef d("12a");
    StringRef e("a12");
    int val;
    to_int(a, val);
    std::cout << val << " ";
    to_int(b, val);
    std::cout << val << " ";
    to_int(c, val);
    std::cout << val << " ";
    std::cout << std::boolalpha << to_int(d, val) << " ";
    std::cout << std::boolalpha << to_int(e, val) << "\n";
}

void test_read_idx_file()
{
    using namespace hana;
    idx::IdxFile idx_file;
    core::Error error = idx::read_idx_file(STR_REF("visus.idx"), &idx_file);
    HANA_ASSERT(error == core::Error::NoError);
}

void test_reverse()
{
    using namespace hana::core;
    int arr[] = { 1, 2, 3, 4, 5, 0, 0, 0 };
    reverse(ArrayRef<int>(arr, 5));
    for (auto v : arr) {
        std::cout << v << " ";
    }
    std::cout << "\n";
    reverse(ArrayRef<int>(arr, 4));
    for (auto v : arr) {
        std::cout << v << " ";
    }
    std::cout << "\n";
}

void test_get_file_name()
{/*
    // Commented out since the function we want to test (get_file_name) is not
    // public. To test it, declare it in the idx.h file.
    using namespace hana;
    idx::IdxFile idx_file;
    core::Error error = idx::read_idx_file(STR_REF("visus.idx"), &idx_file);
    HANA_ASSERT(error == core::Error::NoError);
    uint64_t address =
        0b0101'0100'1100'0100'1101'0100'1010'0101'0101'0011'0001'0011'0101'0010'1001'0110;
    char file_name[512];
    idx::get_file_name(idx_file, 123, address, STR_REF(file_name));
    std::cout << file_name << std::endl;*/
}

void test_interleave_bits()
{
/* // Commented out since the function we want to test (interleave_bits) is not
   // public. To test it, declare it in the idx.h file.
    using namespace hana;
    uint64_t ix = 456;
    uint64_t iy = 124;
    uint64_t iz = 233;
    core::StringRef bit_string = STR_REF("V0012012012012012012012012");
    uint64_t val = idx::interleave_bits(bit_string, ix, iy, iz);
    std::cout << std::bitset<64>(ix) << std::endl;
    std::cout << std::bitset<64>(iy) << std::endl;
    std::cout << std::bitset<64>(iz) << std::endl;
    std::cout << std::bitset<64>(val) << std::endl;*/
}

void test_num_trailing_zeros()
{/* disabled for not compiling on linux
    using namespace hana::core;
    uint64_t v =
        0b1000000000000000'0000000000000000'0000000000000000'0000000000000000;
    std::cout << num_trailing_zeros(v) << std::endl;
    v =
        0b0000000000000000'0000000000000000'0000000000000000'0000000000000000;
    std::cout << num_trailing_zeros(v) << std::endl;
    v =
        0b1000000010000000'0000000000000000'0000000000000000'0000000000000000;
    std::cout << num_trailing_zeros(v) << std::endl;
    v =
        0b1000000010000000'0000000000000000'0000000000000001'0000000000000000;
    std::cout << num_trailing_zeros(v) << std::endl;
    v =
        0b1000000010000000'0000000000000000'0000000000000001'0000001000000000;
    std::cout << num_trailing_zeros(v) << std::endl;
    v =
        0b1000000010000000'0000000000000000'0000000000000001'0000001000000010;
    std::cout << num_trailing_zeros(v) << std::endl;
    v =
        0b1000000010000000'0000000000000000'0000000000000001'0000001000000011;
    std::cout << num_trailing_zeros(v) << std::endl;
*/
}

void test_z_to_hz_address()
{
/* // Commented out since the function we want to test (z_to_hz_address) is not
   // public. To test it, declare it in the idx.h file.
    using namespace hana::idx;
    uint64_t v =
        0b0111111111111111'0000000000000000'1111111111111111'0000000000000000;
    std::cout << std::bitset<64>(z_to_hz_address(v)) << std::endl;
    v =
        0b0111111111111111'0000000000000000'1111111111111111'0000000000000001;
    std::cout << std::bitset<64>(z_to_hz_address(v)) << std::endl;
    v =
        0b0111111111111111'0000000000000000'1111111111111111'0000000000000010;
    std::cout << std::bitset<64>(z_to_hz_address(v)) << std::endl;
    v =
        0b0111111111111111'1111111111111111'1111111111111111'1111111111111101;
    std::cout << std::bitset<64>(z_to_hz_address(v)) << std::endl;
    v =
        0b0000000000000000'0000000000000000'0000000000000000'0000000000000000;
    std::cout << std::bitset<64>(z_to_hz_address(v)) << std::endl;
    */
}

void test_read_idx_grid()
{
    using namespace hana;
    idx::IdxFile idx_file;
    idx::read_idx_file(STR_REF("D:/Dropbox/Projects/nvisusio/build/RelWithDebInfo/heat_release_rate.idx"), &idx_file);
    idx::Grid grid;
    grid.extends = idx_file.box; // get the whole volume
    uint64_t size = idx_file.get_size_inclusive(0, 19);
    core::Mallocator mallocator;
    grid.data = mallocator.allocate(size);
    auto box = idx_file.get_dims_inclusive(19);
    idx::Error error = idx::read_idx_grid_inclusive(idx_file, 0, 0, 19, &grid);
    HANA_ASSERT(error == idx::Error::NoError);
    std::ofstream os("data.raw", std::ios::binary);
    os.write(grid.data.ptr, grid.data.bytes);
    idx::deallocate_memory();
    std::cout << error.code << std::endl;
}

hana::core::Vector3i deinterleave_bits2(hana::core::StringRef bit_string, uint64_t val)
{
    hana::core::Vector3i coord(0, 0, 0);
    const uint64_t one = 1;
    for (int i = 0; i < bit_string.size; ++i) {
        char v = bit_string[i];
        int j = static_cast<int>(bit_string.size) - i - 1;
        if (v == '0') {
            coord.x |= (val & (one << j)) >> j;
            coord.x <<= 1;
        }
        else if (v == '1') {
            coord.y |= (val & (one << j)) >> j;
            coord.y <<= 1;
        }
        else if (v == '2') {
            coord.z |= (val & (one << j)) >> j;
            coord.z <<= 1;
        }
        else {
            HANA_ASSERT(false);
        }
    }
    coord.x >>= 1;
    coord.y >>= 1;
    coord.z >>= 1;
    return coord;
}

template <typename T>
void print(int a)
{
    std::cout << a << ::std::endl;
}

template <typename T>
struct Tem {
    void operator()(int a)
    {
        std::cout << sizeof(T) << " " << a << ::std::endl;
    }
};

template <template <typename> class C, typename S, typename ... A>
auto forward_func(int bytes, A&& ... args)
-> decltype(C<S>()(std::forward<A>(args) ...))
{
    //if ()
    //#define CASE(n) case 8 * (n): return put_block_to_grid<std::array<int8_t, (n)>>(block, grid)
    if (bytes == 0)
        return C<double>()(std::forward<A>(args) ...);
        //return;
    else
        return C<int>()(std::forward<A>(args) ...);
}

bool intersect_grid(const hana::idx::Volume& vol, const hana::core::Vector3i& from,
    const hana::core::Vector3i& stride, OUT hana::core::Vector3i& output_from, OUT hana::core::Vector3i& output_to)
{
    HANA_ASSERT(vol.is_valid());

    output_from = from + ((vol.from + stride - 1 - from) / stride) * stride;
    output_to = from + ((vol.to - from) / stride) * stride;

    // we need to do the following corrections because the behavior of integer
    // division with negative integers are not well defined...
    if (vol.from.x < from.x) { output_from.x = from.x; }
    if (vol.from.y < from.y) { output_from.y = from.y; }
    if (vol.from.z < from.z) { output_from.z = from.z; }
    if (vol.to.x < from.x) { output_to.x = from.x - stride.x; }
    if (vol.to.y < from.y) { output_to.y = from.y - stride.y; }
    if (vol.to.z < from.z) { output_to.z = from.z - stride.z; }

    return output_from <= output_to;
}

void test_intersect_grid()
{
    using namespace hana;
    core::Vector3i from(0, 0, 0);
    core::Vector3i stride(2, 3, 0);
    idx::Volume vol;
    vol.from = core::Vector3i(0, 0, 0);
    vol.to = core::Vector3i(10, 9, 0);
    core::Vector3i output_from, output_to;
    bool result = intersect_grid(vol, from, stride, output_from, output_to);
    HANA_ASSERT(output_from == core::Vector3i(0, 0, 0));
    HANA_ASSERT(output_to == core::Vector3i(10, 9, 0));
    vol.from = core::Vector3i(1, 1, 0);
    vol.to = core::Vector3i(9, 8, 0);
    result = intersect_grid(vol, from, stride, output_from, output_to);
    HANA_ASSERT(output_from == core::Vector3i(2, 3, 0));
    HANA_ASSERT(output_to == core::Vector3i(8, 6, 0));
    vol.from = core::Vector3i(2, 2, 0);
    vol.to = core::Vector3i(8, 5, 0);
    result = intersect_grid(vol, from, stride, output_from, output_to);
    HANA_ASSERT(output_from == core::Vector3i(2, 3, 0));
    HANA_ASSERT(output_to == core::Vector3i(8, 3, 0));
    vol.from = core::Vector3i(1, 4, 0);
    vol.to = core::Vector3i(9, 5, 0);
    result = intersect_grid(vol, from, stride, output_from, output_to);
    HANA_ASSERT(!result);
    vol.from = core::Vector3i(-2, -2, 0);
    vol.to = core::Vector3i(1, 1, 0);
    result = intersect_grid(vol, from, stride, output_from, output_to);
    HANA_ASSERT(output_from == core::Vector3i(0, 0, 0));
    HANA_ASSERT(output_to == core::Vector3i(0, 0, 0));
    vol.from = core::Vector3i(5, 5, 0);
    vol.to = core::Vector3i(5, 5, 0);
    result = intersect_grid(vol, from, stride, output_from, output_to);
    HANA_ASSERT(!result);
    vol.from = core::Vector3i(8, 6, 0);
    vol.to = core::Vector3i(8, 6, 0);
    result = intersect_grid(vol, from, stride, output_from, output_to);
    HANA_ASSERT(output_from == core::Vector3i(8, 6, 0));
    HANA_ASSERT(output_to == core::Vector3i(8, 6, 0));
    vol.from = core::Vector3i(-8, -9, 0);
    vol.to = core::Vector3i(-1, -1, 0);
    result = intersect_grid(vol, from, stride, output_from, output_to);
    HANA_ASSERT(!result);
}

int main()
{
    using namespace hana;
    //test_logger();
    //test_array();
    //test_stream_buf();
    //test_string_tokenizer();
    //test_string_ref();
    //test_idx_file();
    //test_string_to_int();
    //test_read_idx_file();
    //test_reverse();
    test_get_file_name();
    //std::cout << sizeof(idx::IdxFileHeader) << " " << sizeof(idx::IdxBlockHeader) << std::endl;
    //test_interleave_bits();
    //test_num_trailing_zeros();
    //test_z_to_hz_address();
    //std::cout << MAX_PATH << std::endl;
    test_read_idx_grid();
    //test_intersect_grid();
    forward_func<Tem, int>(2, 1);

    return 0;
}
