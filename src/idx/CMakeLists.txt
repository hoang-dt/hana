add_library(idx
            allocator.h array.h bitops.h constants.h debugbreak.h error.h
            filesystem.h io.h logger.h macros.h math.h scope_guard.h streams.h string.h
            time.h types.h utils.h vector.h
            assert.cpp error.cpp filesystem.cpp logger.cpp string.cpp time.cpp
            error.h idx.h idx_block.h idx_file.h types.h utils.h
            error.cpp idx.cpp idx_block.cpp idx_file.cpp types.cpp utils.cpp)
target_link_libraries(idx ${CMAKE_THREAD_LIBS_INIT})

set(IDX_HEADERS
    allocator.h array.h assert.h bitops.h constants.h debugbreak.h
    error.h filesystem.h logger.h io.h macros.h scope_guard.h
    streams.h string.h time.h types.h utils.h vector.h math.h
    error.h idx.h idx_block.h idx_file.h types.h utils.h)
set_target_properties(idx PROPERTIES
    PUBLIC_HEADER "${IDX_HEADERS}"
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 14)
target_include_directories(idx PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>)

if(MSVC)
    target_compile_definitions(idx PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_options(idx PUBLIC /W4)
elseif(UNIX OR APPLE OR MINGW)
    target_compile_options(idx PUBLIC -Wall -Wextra -pedantic)
endif()
target_compile_definitions(idx PUBLIC $<$<CONFIG:DEBUG>:HANA_ASSERT_ON>)

install(TARGETS idx
    EXPORT idx
    ARCHIVE DESTINATION lib/hana
    PUBLIC_HEADER DESTINATION include/hana/idx)
install(EXPORT idx NAMESPACE hana:: DESTINATION lib/hana)

export(EXPORT idx NAMESPACE hana:: FILE ${CMAKE_BINARY_DIR}/idx.cmake)
