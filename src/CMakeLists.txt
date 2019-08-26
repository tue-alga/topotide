file(GLOB CORE_SOURCE *.cpp)

set(SOURCE
	${CORE_SOURCE}
	)

add_subdirectory(gui)

if(BUILD_TESTS)
	add_subdirectory(test)
endif(BUILD_TESTS)

