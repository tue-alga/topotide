file(GLOB TEST_SOURCE *.cpp)

add_executable(ttga-test
	${SOURCE}
	${TEST_SOURCE}
)
target_link_libraries(ttga-test Qt5::Widgets)

if(WITH_KCRASH)
	target_link_libraries(ttga-test KF5::Crash)
endif(WITH_KCRASH)
