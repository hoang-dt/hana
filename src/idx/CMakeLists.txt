add_library(hana_idx error.cpp idx.cpp idx_block.cpp idx_file.cpp types.cpp)
target_link_libraries(hana_idx ${CMAKE_THREAD_LIBS_INIT})

set(HANA_IDX_HEADERS
    error.h idx.h idx_block.h idx_file.h types.h utils.h)
set_target_properties(hana_idx PROPERTIES
    PUBLIC_HEADER "${HANA_IDX_HEADERS}"
	POSITION_INDEPENDENT_CODE ON
    CXX_STANDARD 14)

if(MSVC)
    target_compile_definitions(hana_idx PUBLIC _CRT_SECURE_NO_WARNINGS)
    target_compile_options(hana_idx PUBLIC /W4)
elseif(UNIX OR APPLE OR MINGW)
    target_compile_options(hana_idx PUBLIC -Wall -Wextra -pedantic)
endif()
target_compile_definitions(hana_idx PUBLIC $<$<CONFIG:DEBUG>:HANA_ASSERT_ON>)

install(TARGETS hana_idx
    EXPORT idx
	ARCHIVE DESTINATION ${CMAKE_BINARY_DIR}/lib/hana
	PUBLIC_HEADER DESTINATION ${CMAKE_BINARY_DIR}/include/hana/idx)
install(EXPORT idx NAMESPACE hana DESTINATION ${CMAKE_BINARY_DIR}/lib/hana)
