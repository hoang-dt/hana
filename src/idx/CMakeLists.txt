add_library(idx error.cpp idx.cpp idx_block.cpp idx_file.cpp types.cpp)
set(IDX_HEADERS error.h idx.h idx_block.h idx_file.h types.h utils.h)
set_target_properties(idx PROPERTIES PUBLIC_HEADER "${IDX_HEADERS}"
	POSITION_INDEPENDENT_CODE ON)
install(TARGETS idx
	ARCHIVE DESTINATION "lib/hana/"
	PUBLIC_HEADER DESTINATION "include/hana/idx")

