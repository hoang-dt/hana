cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(hana LANGUAGES CXX C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

find_package(Threads REQUIRED)
find_package(Catch REQUIRED)

# Add doxygen target so we can 'make doc'
find_package(Doxygen)
if (DOXYGEN_FOUND)
	configure_file(doxygen.in ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg @ONLY)
	add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/doxygen.cfg
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation w/ Doxygen" VERBATIM)
endif()

add_subdirectory(src)
