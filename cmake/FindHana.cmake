# Find the Hana (https://bitbucket.org/hoangthaiduong/hana) libraries and headers
# This module defines:
# HANA_FOUND, if false do not try to link against Hana
# HANA_LIBRARIES, the name of the Hana core and idx libraries to link against
# HANA_INCLUDE_DIR, the location of the Hana headers
#
# You can also specify the environment variable HANA_DIR or define it with
# -DHANA_DIR=... to hint at the module where to search for Hana if it's
# installed in a non-standard location.

# Find the include for Hana
find_path(HANA_INCLUDE_DIR idx/idx.h core/types.h
	HINTS
	$ENV{HANA_DIR}
	${HANA_DIR}
	PATH_SUFFIXES hana include/hana include
	PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/usr/local/include/
	/usr/include/
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

find_library(HANA_CORE_LIBRARY NAMES core
	HINTS
	$ENV{HANA_DIR}
	${HANA_DIR}
	PATH_SUFFIXES hana lib lib/hana
	PATHS
	/sw
	/opt/local
	/opt/csw
	/opt
)

find_library(HANA_IDX_LIBRARY NAMES idx
	HINTS
	$ENV{HANA_DIR}
	${HANA_DIR}
	PATH_SUFFIXES hana lib lib/hana
	PATHS
	/sw
	/opt/local
	/opt/csw
	/opt
)
set(HANA_FOUND FALSE)
if (HANA_CORE_LIBRARY AND HANA_IDX_LIBRARY)
	set(HANA_LIBRARIES ${HANA_IDX_LIBRARY} ${HANA_CORE_LIBRARY}
		CACHE STRING "The Hana libraries")
	set(HANA_CORE_LIBRARY ${HANA_CORE_LIBRARY} CACHE INTERNAL "")
	set(HANA_IDX_LIBRARY ${HANA_IDX_LIBRARY} CACHE INTERNAL "")
	set(HANA_FOUND TRUE)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Hana REQUIRED_VARS HANA_LIBRARIES
	HANA_INCLUDE_DIR HANA_CORE_LIBRARY HANA_IDX_LIBRARY)


