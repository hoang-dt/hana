#include <idx/math.h>
#include <idx/idx.h>
#include <idx/idx_file.h>
#include "md5.h"
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>

using namespace hana;
using namespace std;

// Read the entire volume at full resolution. Reading the data at full
// resolution always work in "progressive" mode (data in hz levels
// 0, 1, 2, ..., max_hz are merged together).
void test_read_idx_grid_1()
{
    cout << "Test 1" << endl;

    IdxFile idx_file;

    Error error = read_idx_file("../../../data/flame_small_heat.idx", &idx_file);
    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level();
    int field = idx_file.get_field_index("heat");
    int time = idx_file.get_min_time_step();

    Grid grid;
    grid.extent = idx_file.get_logical_extent();
    grid.data.bytes = idx_file.get_size_inclusive(grid.extent, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid.extent, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
    deallocate_memory();

    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    string hash = md5(grid.data.ptr, (long) grid.data.bytes);
    cout << "MD5 = " << hash<< "\n";
    HANA_ASSERT(hash == "b17f827b14d064cf1913dec906484733");

    free(grid.data.ptr);
}

// Read the entire volume at one-fourth the full resolution in "progressive"
// mode (data in hz levels 0, 1, 2, ..., (max_hz - 2) are merged together).
void test_read_idx_grid_2()
{
    cout << "Test 2" << endl;

    IdxFile idx_file;

    Error error = read_idx_file("../../../data/flame_small_heat.idx", &idx_file);
    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level() - 2;
    int field = idx_file.get_field_index("heat");
    int time = idx_file.get_max_time_step();

    Grid grid;
    grid.extent = idx_file.get_logical_extent();
    grid.data.bytes = idx_file.get_size_inclusive(grid.extent, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid.extent, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
    deallocate_memory();

    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    string hash = md5(grid.data.ptr, (long) grid.data.bytes);
    cout << "MD5 = " << hash<< "\n";
    HANA_ASSERT(hash == "3b7afc6f392b17310eaf624dc271428e");

    free(grid.data.ptr);
}

// Read the entire volume at one-fourth the full resolution by directly
// querying data from hz level (max_hz - 1). Note that this is a different set
// of data than in the progressive case (although the resulting grid has the
// same dimensions).
void test_read_idx_grid_3()
{
    cout << "Test 3" << endl;

    IdxFile idx_file;

    Error error = read_idx_file("../../../data/flame_small_heat.idx", &idx_file);
    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level() - 1;
    int field = idx_file.get_field_index("heat");
    int time = idx_file.get_max_time_step();

    Grid grid;
    grid.extent = idx_file.get_logical_extent();
    grid.data.bytes = idx_file.get_size(grid.extent, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid(grid.extent, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid(idx_file, field, time, hz_level, &grid);
    deallocate_memory();

    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    string hash = md5(grid.data.ptr, (long) grid.data.bytes);
    cout << "MD5 = " << hash<< "\n";
    HANA_ASSERT(hash == "f25a0711703c6b1ad75945045a87bb70");

    free(grid.data.ptr);
}

// Read a portion of the volume at half resolution. Note that by using the
// maximum hz level in non-progressive mode, we are getting only half-resolution
// data.
void test_read_idx_grid_4()
{
    cout << "Test 4" << endl;

    IdxFile idx_file;

    Error error = read_idx_file("../../../data/flame_small_o2.idx", &idx_file);
    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level();
    int field = idx_file.get_field_index("o2");
    int time = idx_file.get_min_time_step();

    Grid grid;
    grid.extent.from = Vector3i(30, 0, 0);
    grid.extent.to = Vector3i(100, 63, 63);
    grid.data.bytes = idx_file.get_size(grid.extent, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid(grid.extent, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid(idx_file, field, time, hz_level, &grid);
    deallocate_memory();

    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    string hash = md5(grid.data.ptr, (long) grid.data.bytes);
    cout << "MD5 = " << hash<< "\n";
    HANA_ASSERT(hash == "18d0d779c5e1395c077476f1a6da53e5");

    free(grid.data.ptr);
}

// Read a slice of the volume at full resolution.
void test_read_idx_grid_5()
{
    cout << "Test 5" << endl;

    IdxFile idx_file;

    Error error = read_idx_file("../../../data/flame_small_o2.idx", &idx_file);
    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level();
    int field = idx_file.get_field_index("o2");
    int time = idx_file.get_min_time_step();

    Grid grid;
    grid.extent.from = Vector3i(70, 0, 0);
    grid.extent.to = Vector3i(70, 63, 63);
    grid.data.bytes = idx_file.get_size_inclusive(grid.extent, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid.extent, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
    deallocate_memory();

    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    string hash = md5(grid.data.ptr, (long)grid.data.bytes);
    cout << "MD5 = " << hash << "\n";
    HANA_ASSERT(hash == "107a1d8b1107130965e783f4f8fcf340");

    free(grid.data.ptr);
}

// Read at a very low hz level (row major order, inclusive).
void test_read_idx_grid_6()
{
    cout << "Test 6" << endl;

    IdxFile idx_file;

    Error error = read_idx_file("../../../data/flame_small_o2.idx", &idx_file);
    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = 3;
    int field = idx_file.get_field_index("o2");
    int time = idx_file.get_min_time_step();

    Grid grid;
    grid.extent.from = Vector3i(0, 0, 0);
    grid.extent.to = Vector3i(63, 63, 63);
    grid.data.bytes = idx_file.get_size_inclusive(grid.extent, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid.extent, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
    deallocate_memory();

    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    string hash = md5(grid.data.ptr, (long)grid.data.bytes);
    cout << "MD5 = " << hash << "\n";
    HANA_ASSERT(hash == "b9499e518ce143296c9cdabd66b27a04");

    free(grid.data.ptr);
}

// Read at a very low hz level (hz order, non-inclusive).
void test_read_idx_grid_7()
{
    cout << "Test 7" << endl;

    IdxFile idx_file;

    Error error = read_idx_file("../../../data/flame_small_heat.idx", &idx_file);
    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = 4;
    int field = idx_file.get_field_index("heat");
    int time = idx_file.get_max_time_step();

    Grid grid;
    grid.extent = idx_file.get_logical_extent();
    grid.data.bytes = idx_file.get_size(grid.extent, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid(grid.extent, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid(idx_file, field, time, hz_level, &grid);
    deallocate_memory();

    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    string hash = md5(grid.data.ptr, (long)grid.data.bytes);
    cout << "MD5 = " << hash << "\n";
    HANA_ASSERT(hash == "dc266a8556b763d40a1bdde0a2d040ee");

    free(grid.data.ptr);
}

void performance_test()
{
    auto begin = clock();

    cout << "Performance test" << endl;

    IdxFile idx_file;

    Error error = read_idx_file("../../../data/magnetic_reconnection.idx", &idx_file);
    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level();
    int field = idx_file.get_field_index("value");
    int time = 0;

    Grid grid;
    grid.extent = idx_file.get_logical_extent();
    grid.data.bytes = idx_file.get_size_inclusive(grid.extent, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid.extent, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
    deallocate_memory();

    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    free(grid.data.ptr);

    auto end = clock();
    auto elapsed = (end - begin) / (float)CLOCKS_PER_SEC;
    cout << "Elapsed time = " << elapsed << "s\n";
}

void test_read_idx_grid_8()
{
    cout << "Test 8" << endl;

    IdxFile idx_file;

    Error error = read_idx_file("../../../data/blob/blob.idx", &idx_file);
    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level();
    //int field = idx_file.get_field_index("heat");
    int field = 0;
    int time = idx_file.get_min_time_step();

    Grid grid;
    //grid.extent = idx_file.get_logical_extent();
    grid.extent.from = Vector3i(0, 0, 0);
    grid.extent.to = Vector3i(24, 24, 25);
    grid.data.bytes = idx_file.get_size_inclusive(grid.extent, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid.extent, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
    deallocate_memory();

    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    string hash = md5(grid.data.ptr, (long)grid.data.bytes);

    HANA_ASSERT(hash == "423dc67376d85671419ee10e2a35e54d");
    free(grid.data.ptr);
}

void test_read_idx_grid_9()
{
    cout << "Test 9" << endl;

    IdxFile idx_file;

    Error error = read_idx_file("../../../data/blob/blob.idx", &idx_file);
    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level();
    //int field = idx_file.get_field_index("heat");
    int field = 0;
    int time = idx_file.get_min_time_step();

    Grid grid;
    //grid.extent = idx_file.get_logical_extent();
    grid.extent.from = Vector3i(25, 0, 0);
    grid.extent.to = Vector3i(49, 25, 25);
    grid.data.bytes = idx_file.get_size_inclusive(grid.extent, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid.extent, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
    deallocate_memory();

    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    string hash = md5(grid.data.ptr, (long)grid.data.bytes);
    HANA_ASSERT(hash == "8e74b8324940391c977009af96580b16");
    cout << "MD5 = " << hash << "\n";

    free(grid.data.ptr);
}

void test_read_idx_grid_10()
{
    cout << "Test 10" << endl;

    IdxFile idx_file;

    Error error = read_idx_file("../../../data/blob200.idx", &idx_file);
    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level();
    int field = 0;
    int time = idx_file.get_min_time_step();

    Grid grid;
    grid.extent.from = Vector3i(100, 100, 100);
    grid.extent.to = Vector3i(199, 199, 199);
    grid.data.bytes = idx_file.get_size_inclusive(grid.extent, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid.extent, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
    deallocate_memory();

    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    string hash = md5(grid.data.ptr, (long)grid.data.bytes);
    cout << "MD5 = " << hash << "\n";

    free(grid.data.ptr);
}

void test_read_idx_grid_11()
{
    cout << "Test 11" << endl;

    IdxFile idx_file;

    Error error = read_idx_file("../../../data/2x2.idx", &idx_file);
    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level();
    int field = 0;
    int time = idx_file.get_min_time_step();

    Grid grid;
    grid.extent.from = Vector3i(0, 0, 0);
    grid.extent.to = Vector3i(3, 3, 0);
    grid.data.bytes = idx_file.get_size_inclusive(grid.extent, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid.extent, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
    deallocate_memory();

    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    string hash = md5(grid.data.ptr, (long)grid.data.bytes);
    cout << "MD5 = " << hash << "\n";

    free(grid.data.ptr);
}

// Test decompression
void test_read_idx_grid_12()
{
    cout << "Test 12" << endl;

    IdxFile idx_file;

    Error error = read_idx_file("../../../data/bonsai_zlib.idx", &idx_file);
    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level();
    int field = 0;
    int time = 0;

    Grid grid;
    grid.extent = idx_file.get_logical_extent();
    grid.data.bytes = idx_file.get_size_inclusive(grid.extent, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid.extent, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
    deallocate_memory();

    if (error.code != Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    string hash = md5(grid.data.ptr, (long)grid.data.bytes);
    ofstream output("out.raw", ios::binary);
    output.write(grid.data.ptr, grid.data.bytes);
    cout << "MD5 = " << hash << "\n";

    free(grid.data.ptr);
}

int main()
{
    using namespace hana;

    //test_read_idx_grid_1();
    //test_read_idx_grid_2();
    //test_read_idx_grid_3();
    //test_read_idx_grid_4();
    //test_read_idx_grid_5();
    //test_read_idx_grid_6();
    ////test_read_idx_grid_7(); // TODO: fix this test (currently reading resolutions less than the min hz level can only be done in "inclusive" mode)
    //test_read_idx_grid_8();
    //test_read_idx_grid_9();
    //test_read_idx_grid_10();
    ////test_read_idx_grid_11();
    test_read_idx_grid_12();
    //performance_test();
    return 0;
}
