add_executable(topotide
	backgrounddock.cpp
	backgroundthread.cpp
	colorramp.cpp
	coordinatelabel.cpp
	graphwriter.cpp
	linksequencewriter.cpp
	mergetreedock.cpp
	progressdock.cpp
	riverapp.cpp
	rivercli.cpp
	riverdata.cpp
	rivergui.cpp
	riverwidget.cpp
	settingsdock.cpp
	timedock.cpp
	uihelper.cpp
	unitsdock.cpp
	unitshelper.cpp
	resources.qrc
)
target_link_libraries(topotide PRIVATE topotidelib)
target_link_libraries(topotide PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::OpenGL Qt6::OpenGLWidgets)

install(TARGETS topotide DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
