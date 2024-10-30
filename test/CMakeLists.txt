file(GLOB TEST_SOURCE *.cpp io/*.cpp)

add_executable(topotide_test
	${TEST_SOURCE}
)

target_link_libraries(topotide_test PRIVATE topotidelib)
target_link_libraries(topotide_test PUBLIC Qt6::Core Qt6::Gui)
