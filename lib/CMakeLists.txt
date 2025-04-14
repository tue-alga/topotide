set(TOPOTIDELIB_SOURCE
	boundary.cpp
	boundarycreator.cpp
	boundaryreader.cpp
	boundarywriter.cpp
	heightmap.cpp
	inputdcel.cpp
	inputgraph.cpp
	linksequence.cpp
	mergetree.cpp
	mscomplex.cpp
	mscomplexcreator.cpp
	mscomplexsimplifier.cpp
	mstonetworkgraphcreator.cpp
	networkgraph.cpp
	path.cpp
	piecewiselinearfunction.cpp
	point.cpp
	unionfind.cpp
	units.cpp
	io/esrigridreader.cpp
	io/esrigridwriter.cpp
	io/gdalreader.cpp
	io/textfilereader.cpp
)

if(EXPERIMENTAL_FINGERS_SUPPORT)
	set(TOPOTIDELIB_SOURCE ${TOPOTIDELIB_SOURCE}
		fingerfinder.cpp
		gradientfieldsimplifier.cpp
	)
endif()

add_library(topotidelib ${TOPOTIDELIB_SOURCE})
target_link_libraries(topotidelib PRIVATE Qt6::Gui)
target_link_libraries(topotidelib PRIVATE GDAL::GDAL)
target_include_directories(topotidelib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
