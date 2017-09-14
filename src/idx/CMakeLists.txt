add_library(hana
            allocator.h array.h bitops.h constants.h debugbreak.h error.h
            filesystem.h io.h logger.h macros.h math.h scope_guard.h streams.h string.h
            time.h types.h utils.h vector.h miniz.h
            assert.cpp error.cpp filesystem.cpp logger.cpp string.cpp time.cpp
            error.h idx.h idx_block.h idx_common.h idx_file.h types.h utils.h
            error.cpp idx.cpp idx_block.cpp idx_common.cpp idx_file.cpp idx_write.cpp
            types.cpp utils.cpp miniz.c)
target_link_libraries(hana ${CMAKE_THREAD_LIBS_INIT})

set(IDX_HEADERS
    allocator.h array.h assert.h bitops.h constants.h debugbreak.h
    error.h filesystem.h logger.h io.h macros.h scope_guard.h
    streams.h string.h time.h types.h utils.h vector.h math.h
    error.h idx.h idx_block.h idx_file.h idx_common.h timer.h types.h utils.h)
set_target_properties(hana PROPERTIES
    PUBLIC_HEADER "${IDX_HEADERS}"
    POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 14)
target_include_directories(hana PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>)

# compiler specifics
if(MSVC)
    target_compile_definitions(hana PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_options(hana PUBLIC /W4)
elseif(UNIX OR APPLE OR MINGW)
    target_compile_options(hana PUBLIC -Wall -Wextra -pedantic)
endif()
target_compile_definitions(hana PUBLIC $<$<CONFIG:DEBUG>:HANA_ASSERT_ON>)

# install
install(TARGETS hana
    EXPORT hana-config
    ARCHIVE DESTINATION lib/hana
    PUBLIC_HEADER DESTINATION include/hana)

install(EXPORT hana-config DESTINATION lib/hana)
