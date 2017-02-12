# Find Blosc (https://github.com/Blosc/c-blosc).

include(ExternalProject)
ExternalProject_Add(
    BLOSC
    GIT_REPOSITORY "https://github.com/hoangthaiduong/c-blosc.git"
    GIT_TAG "master"
    CMAKE_ARGS -DPREFER_EXTERNAL_ZSTD:BOOL=FALSE
               -DPREFER_EXTERNAL_ZLIB:BOOL=FALSE
               -DPREFER_EXTERNAL_SNAPPY:BOOL=FALSE
               -DPREFER_EXTERNAL_LZ4:BOOL=FALSE
               -DCMAKE_INSTALL_PREFIX:STRING=${CMAKE_SOURCE_DIR}/external/c-blosc/install
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/c-blosc"
)
set(BLOSC_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/c-blosc/blosc")
find_package_handle_standard_args(BLOSC REQUIRED_VARS BLOSC_INCLUDE_DIR)
