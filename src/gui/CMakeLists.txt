file(GLOB GUI_SOURCE *.cpp)

add_executable(ttga
	${SOURCE}
	${GUI_SOURCE}
	resources.qrc
)
target_link_libraries(ttga Qt5::Widgets)

if(WITH_KCRASH)
	target_link_libraries(ttga KF5::Crash)
endif(WITH_KCRASH)

if(WITH_IPELIB)
	target_link_libraries(ttga ${Ipelib_LIBRARIES})
endif(WITH_IPELIB)

install(TARGETS ttga DESTINATION bin)

