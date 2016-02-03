#include "../idx/idx.h"
#include "../idx/idx_file.h"
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>

using namespace hana;
using namespace hana::core;
using namespace hana::idx;
using namespace std;

// Read the entire volume at full resolution. Reading the data at full
// resolution always work in "progressive" mode (data in hz levels
// 0, 1, 2, ..., max_hz are merged together).
void test_read_idx_grid_1()
{
    cout << "Test 1" << endl;

    IdxFile idx_file;

    idx::Error error = read_idx_file("../../data/flame_small_heat.idx", &idx_file);
    if (error.code != core::Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level();
    int field = idx_file.get_field_index("heat");
    int time = idx_file.get_min_time_step();

    Grid grid;
    grid.extends = idx_file.get_logical_extends();
    grid.data.bytes = idx_file.get_size_inclusive(grid.extends, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid.extends, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
    idx::deallocate_memory();

    if (error.code != core::Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    ofstream os("data1.raw", ios::binary);
    os.write(grid.data.ptr, grid.data.bytes);

    free(grid.data.ptr);
}

// Read the entire volume at one-fourth the full resolution in "progressive"
// mode (data in hz levels 0, 1, 2, ..., (max_hz - 2) are merged together).
void test_read_idx_grid_2()
{
    cout << "Test 2" << endl;

    IdxFile idx_file;

    idx::Error error = read_idx_file("../../data/flame_small_heat.idx", &idx_file);
    if (error.code != core::Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level() - 2;
    int field = idx_file.get_field_index("heat");
    int time = idx_file.get_max_time_step();

    Grid grid;
    grid.extends = idx_file.get_logical_extends();
    grid.data.bytes = idx_file.get_size_inclusive(grid.extends, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid.extends, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
    idx::deallocate_memory();

    if (error.code != core::Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    ofstream os("data2.raw", ios::binary);
    os.write(grid.data.ptr, grid.data.bytes);

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

    idx::Error error = read_idx_file("../../data/flame_small_heat.idx", &idx_file);
    if (error.code != core::Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level() - 1;
    int field = idx_file.get_field_index("heat");
    int time = idx_file.get_max_time_step();

    Grid grid;
    grid.extends = idx_file.get_logical_extends();
    grid.data.bytes = idx_file.get_size(grid.extends, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid(grid.extends, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid(idx_file, field, time, hz_level, &grid);
    idx::deallocate_memory();

    if (error.code != core::Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    ofstream os("data3.raw", ios::binary);
    os.write(grid.data.ptr, grid.data.bytes);

    free(grid.data.ptr);
}

// Read a portion of the volume at half resolution. Note that by using the
// maximum hz level in non-progressive mode, we are getting only half-resolution
// data.
void test_read_idx_grid_4()
{
    cout << "Test 4" << endl;

    IdxFile idx_file;

    idx::Error error = read_idx_file("../../data/flame_small_o2.idx", &idx_file);
    if (error.code != core::Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level();
    int field = idx_file.get_field_index("o2");
    int time = idx_file.get_min_time_step();

    Grid grid;
    grid.extends.from = Vector3i(30, 0, 0);
    grid.extends.to = Vector3i(100, 63, 63);
    grid.data.bytes = idx_file.get_size(grid.extends, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid(grid.extends, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid(idx_file, field, time, hz_level, &grid);
    idx::deallocate_memory();

    if (error.code != core::Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    ofstream os("data4.raw", ios::binary);
    os.write(grid.data.ptr, grid.data.bytes);

    free(grid.data.ptr);
}

// Read a slice of the volume at full resolution.
void test_read_idx_grid_5()
{
    cout << "Test 5" << endl;

    IdxFile idx_file;

    idx::Error error = read_idx_file("../../data/flame_small_o2.idx", &idx_file);
    if (error.code != core::Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level();
    int field = idx_file.get_field_index("o2");
    int time = idx_file.get_min_time_step();

    Grid grid;
    grid.extends.from = Vector3i(70, 0, 0);
    grid.extends.to = Vector3i(70, 63, 63);
    grid.data.bytes = idx_file.get_size_inclusive(grid.extends, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid.extends, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
    idx::deallocate_memory();

    if (error.code != core::Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    ofstream os("data5.raw", ios::binary);
    os.write(grid.data.ptr, grid.data.bytes);

    free(grid.data.ptr);
}

void performance_test()
{
    auto begin = clock();

    cout << "Performance test" << endl;

    IdxFile idx_file;

    idx::Error error = read_idx_file("../../data/magnetic_reconnection.idx", &idx_file);
    if (error.code != core::Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    int hz_level = idx_file.get_max_hz_level();
    int field = idx_file.get_field_index("value");
    int time = 0;

    Grid grid;
    grid.extends = idx_file.get_logical_extends();
    grid.data.bytes = idx_file.get_size_inclusive(grid.extends, field, hz_level);
    grid.data.ptr = (char*)malloc(grid.data.bytes);

    Vector3i from, to, stride;
    idx_file.get_grid_inclusive(grid.extends, hz_level, from, to, stride);
    Vector3i dim = (to - from) / stride + 1;
    cout << "Resulting grid dim = " << dim.x << " x " << dim.y << " x " << dim.z << "\n";

    error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
    idx::deallocate_memory();

    if (error.code != core::Error::NoError) {
        cout << "Error: " << error.get_error_msg() << "\n";
        return;
    }

    free(grid.data.ptr);

    auto end = clock();
    auto elapsed = (end - begin) / (float)CLOCKS_PER_SEC;
    cout << "Elapsed time = " << elapsed << "s\n";
}

int main()
{
    //test_read_idx_grid_1();
    //test_read_idx_grid_2();
    //test_read_idx_grid_3();
    //test_read_idx_grid_4();
    //test_read_idx_grid_5();
    performance_test();
    return 0;
}
