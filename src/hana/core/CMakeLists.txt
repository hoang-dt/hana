add_library(core
            allocator.h array.h bitops.h constants.h debugbreak.h error.h
            filesystem.h io.h logger.h macros.h math.h scope_guard.h streams.h string.h
            time.h types.h utils.h vector.h
            assert.cpp error.cpp filesystem.cpp logger.cpp string.cpp time.cpp)
target_link_libraries(core ${CMAKE_THREAD_LIBS_INIT})

set(CORE_HEADERS
    allocator.h array.h assert.h bitops.h constants.h debugbreak.h
	error.h filesystem.h logger.h macros.h scope_guard.h
	streams.h string.h time.h types.h utils.h vector.h)
set_target_properties(core PROPERTIES
    PUBLIC_HEADER "${CORE_HEADERS}"
	POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 14)
target_include_directories(core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:include>)

if(MSVC)
    target_compile_definitions(core PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_options(core PUBLIC /W4)
elseif(UNIX OR APPLE OR MINGW)
    target_compile_options(core PUBLIC -Wall -Wextra -pedantic)
endif()
target_compile_definitions(core PUBLIC $<$<CONFIG:DEBUG>:HANA_ASSERT_ON>)

install(TARGETS core
    EXPORT core
	ARCHIVE DESTINATION lib/hana
	PUBLIC_HEADER DESTINATION include/hana/core)
install(EXPORT core NAMESPACE hana:: DESTINATION lib/hana)

export(EXPORT core NAMESPACE hana:: FILE ${CMAKE_BINARY_DIR}/core.cmake)
