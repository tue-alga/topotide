include(FindPackageHandleStandardArgs)

find_path(Ipelib_INCLUDE_DIR NAMES ipelib.h)

find_library(Ipelib_LIBRARY NAMES ipe)

find_package_handle_standard_args(Ipelib
	DEFAULT_MSG Ipelib_LIBRARY Ipelib_INCLUDE_DIR)

if (Ipelib_FOUND)
	set(Ipelib_LIBRARIES ${Ipelib_LIBRARY})
	set(Ipelib_INCLUDE_DIRS ${Ipelib_INCLUDE_DIR})
endif (Ipelib_FOUND)

mark_as_advanced(Ipelib_LIBRARY Ipelib_INCLUDE_DIR)
