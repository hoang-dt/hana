### What is this library for? ###
Hana is a library to work with the IDX binary format. Currently it can read IDX data from disk and store it as a regular gird in memory in row-major order. The inputs are: the path to the IDX metadata file (.idx), the field's (or variable's) name, the time step, the resolution level, and the extents of the 3D volume you want to extract. 

### How to use the library? ###
Use the provided CMake file to build the library. You can do `make install` on Linux, or build the INSTALL target in Visual Studio to install the library. This will also install the necessary .cmake files for you to import into your own CMake project. To use the library, include the `idx_file.h` and `idx.h` header files and link your project with the two libraries `core` and `idx`. Hana is written in C++11 so your compiler's version needs to be recent (I have tested with Visual Studio 2015 Update 1 and GCC 4.8).

Examples on how to use the API can be found in the `src/hana/tests` directory. To run the test program, you need to compile it (see the paragraph below). You will also want to download the test data from [here](https://www.dropbox.com/s/custbwkmedyht66/data.zip?dl=0).

You can run doxygen in the main directory to generate documentation in the `doc` directory.

Below is a simple code example for reading the entire volume at full resolution, without error checking. Please take a look at the `src/test` directory for more complete examples.

```c++
IdxFile idx_file;

read_idx_file(STR_REF("../../data/flame_small_heat.idx"), &idx_file);

int hz_level = idx_file.get_max_hz_level();
int field = idx_file.get_field_index("heat");
int time = idx_file.get_min_time_step();

Grid grid;
grid.extents = idx_file.get_logical_extends();
grid.data.bytes = idx_file.get_size_inclusive(grid.extends, field, hz_level);
grid.data.ptr = (char*)malloc(grid.data.bytes);

error = read_idx_grid_inclusive(idx_file, field, time, hz_level, &grid);
idx::deallocate_memory();
free(grid.data.ptr);
```