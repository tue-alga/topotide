set(TOPOTIDELIB_SOURCE
	boundary.cpp
	boundaryreader.cpp
	boundarywriter.cpp
	heightmap.cpp
	inputdcel.cpp
	inputgraph.cpp
	linksequence.cpp
	mscomplex.cpp
	mscomplexcreator.cpp
	mscomplexsimplifier.cpp
	mstonetworkgraphcreator.cpp
	networkgraph.cpp
	path.cpp
	piecewisecubicfunction.cpp
	point.cpp
	unionfind.cpp
	units.cpp
	io/esrigridreader.cpp
	io/gdalreader.cpp
	io/textfilereader.cpp
)

add_library(topotidelib ${TOPOTIDELIB_SOURCE})
target_link_libraries(topotidelib PRIVATE Qt6::Gui)
target_link_libraries(topotidelib PRIVATE GDAL::GDAL)
target_include_directories(topotidelib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

install(TARGETS topotidelib DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/topotide)
install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/topotide)
