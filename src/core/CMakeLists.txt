add_library(hana_core assert.cpp error.cpp filesystem.cpp logger.cpp string.cpp time.cpp)
target_link_libraries(hana_core ${CMAKE_THREAD_LIBS_INIT})

set(HANA_CORE_HEADERS
    allocator.h array.h assert.h bitops.h constants.h debugbreak.h
	error.h filesystem.h logger.h macros.h scope_guard.h
	streams.h string.h time.h types.h utils.h vector.h)
set_target_properties(hana_core PROPERTIES
    PUBLIC_HEADER "${HANA_CORE_HEADERS}"
	POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 14)

if(MSVC)
    target_compile_definitions(hana_core PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_options(hana_core PUBLIC /W4)
elseif(UNIX OR APPLE OR MINGW)
    target_compile_options(hana_core PUBLIC -Wall -Wextra -pedantic)
endif()
target_compile_definitions(hana_core PUBLIC $<$<CONFIG:DEBUG>:HANA_ASSERT_ON>)

install(TARGETS hana_core
    EXPORT core
	ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/lib/hana
	PUBLIC_HEADER DESTINATION ${CMAKE_BINARY_DIR}/include/hana/core)
install(EXPORT core NAMESPACE hana DESTINATION ${CMAKE_BINARY_DIR}/lib/hana)
