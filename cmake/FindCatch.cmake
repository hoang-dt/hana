# Find the Catch test framework (https://github.com/philsquared/Catch).

find_path(CATCH_INCLUDE_DIR catch.hpp
	HINTS ${CATCH}
	$ENV{CATCH}
	PATH_SUFFIXES catch include
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CATCH REQUIRED_VARS CATCH_INCLUDE_DIR)

if(NOT CATCH_FOUND)    
    include(ExternalProject)
    ExternalProject_Add(
        Catch
        GIT_REPOSITORY "https://bitbucket.org/hoangthaiduong/catch"
        GIT_TAG "master"
        UPDATE_COMMAND ""
        PATCH_COMMAND ""
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        TEST_COMMAND ""
        INSTALL_COMMAND ""
        SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/Catch"
    )
    set(CATCH_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/external/Catch/single_include")
    find_package_handle_standard_args(CATCH REQUIRED_VARS CATCH_INCLUDE_DIR)    
endif()
