add_library(core assert.cpp error.cpp filesystem.cpp logger.cpp string.cpp time.cpp)
target_link_libraries(core ${CMAKE_THREAD_LIBS_INIT})
set(CORE_HEADERS allocator.h array.h assert.h bitops.h constants.h debugbreak.h
	error.h filesystem.h logger.h macros.h scope_guard.h
	streams.h string.h time.h types.h utils.h vector.h)
set_target_properties(core PROPERTIES PUBLIC_HEADER "${CORE_HEADERS}"
	POSITION_INDEPENDENT_CODE ON)

install(TARGETS core
	ARCHIVE DESTINATION "lib/hana/"
	PUBLIC_HEADER DESTINATION "include/hana/core")
