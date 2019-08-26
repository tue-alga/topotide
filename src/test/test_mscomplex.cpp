#include <QColor>
#include <QImage>

#include <limits>

#include "catch.hpp"

#include "../inputgraph.h"
#include "../mscomplex.h"
#include "../mscomplexcreator.h"

TEST_CASE("creating a Morse-Smale complex") {

	// create some heightmaps with the same height everywhere
	int size;
	SECTION("2x2") {
		size = 2;
	}
	SECTION("3x3") {
		size = 3;
	}
	SECTION("5x5") {
		size = 5;
	}
	SECTION("10x10") {
		size = 10;
	}
	QImage image = QImage(size, size, QImage::Format_RGB32);
	image.fill(QColor("black"));
	HeightMap heightMap(image);

	InputGraph g(heightMap);
	InputDcel dcel(g);
	MsComplex msc;
	MsComplexCreator msCreator(dcel, &msc);
	msCreator.create();

	// this should produce two minima (the source and the sink), and one saddle
	// (the point on (1, 0) that is connected to the global maximum, the sink,
	// the point on (1, 1) that is higher because its y-coordinate is higher
	// and the point on (0, 0) that is lower because its x-coordinate is lower)
	REQUIRE(msc.vertexCount() == 3);

	// there should be two MS edges, from the saddle to the two minima
	REQUIRE(msc.halfEdgeCount() == 2 * 2);

	// only the outer face
	REQUIRE(msc.faceCount() == 1);

	// the number of DCEL triangles inside it
	REQUIRE(msc.face(0).data().triangles.size() ==
			2 * (size - 1) * (size - 1) +
			2 * (size - 1) +
			2 * (size - 1) +
			4);

	// the maximum should be the global maximum
	REQUIRE(msc.face(0).data().maximum == dcel.vertex(2));

	// the persistence of this maximum should be infinity
	REQUIRE(msc.face(0).data().persistence ==
	        std::numeric_limits<double>::infinity());
}

TEST_CASE("creating a Morse-Smale complex with a saddle on a "
			"minimum -> saddle path") {

	InputGraph g;
	for (int i = 0; i < 11; i++) g.addVertex();

	g[0].p = Point(0, 2, 0);
	g[0].addAdjacency(8);
	g[0].addAdjacency(1);
	g[0].addAdjacency(5);

	g[1].p = Point(1, 2, 2);
	g[1].addAdjacency(3);
	g[1].addAdjacency(2);
	g[1].addAdjacency(4);
	g[1].addAdjacency(0);

	g[2].p = Point(2, 2, 1);
	g[2].addAdjacency(1);
	g[2].addAdjacency(9);
	g[2].addAdjacency(6);

	g[3].p = Point(1, 3, 3);
	g[3].addAdjacency(8);
	g[3].addAdjacency(10);
	g[3].addAdjacency(9);
	g[3].addAdjacency(1);

	g[4].p = Point(1, 1, 3);
	g[4].addAdjacency(6);
	g[4].addAdjacency(7);
	g[4].addAdjacency(5);
	g[4].addAdjacency(1);

	g[5].p = Point(0, 1, 4);
	g[5].addAdjacency(7);
	g[5].addAdjacency(0);
	g[5].addAdjacency(4);

	g[6].p = Point(2, 1, 4);
	g[6].addAdjacency(7);
	g[6].addAdjacency(4);
	g[6].addAdjacency(2);

	g[7].p = Point(1, 0, 2);
	g[7].addAdjacency(5);
	g[7].addAdjacency(4);
	g[7].addAdjacency(6);

	g[8].p = Point(0, 3, 4);
	g[8].addAdjacency(10);
	g[8].addAdjacency(3);
	g[8].addAdjacency(0);

	g[9].p = Point(2, 3, 4);
	g[9].addAdjacency(2);
	g[9].addAdjacency(3);
	g[9].addAdjacency(10);

	g[10].p = Point(1, 4, 2);
	g[10].addAdjacency(9);
	g[10].addAdjacency(3);
	g[10].addAdjacency(8);

	InputDcel dcel(g);
	MsComplex msc;
	MsComplexCreator msCreator(dcel, &msc);
	msCreator.create();

	// this should produce four minima (0, 2, 7, 10), and three saddles
	// (1, 3, 4)
	REQUIRE(msc.vertexCount() == 7);

	// there should be six MS edges
	// (0 -> 1, 0 -> 3, 0 -> 4, 2 -> 1, 7 -> 4, 10 -> 3)
	REQUIRE(msc.halfEdgeCount() == 6 * 2);

	// only the outer face
	REQUIRE(msc.faceCount() == 1);

	// TODO check whether the saddles are matched to 0, not 2
	// (because 1 -> 0 is steeper than 1 -> 2)
}

TEST_CASE("Morse-Smale complex on a monkey saddle") {

	InputGraph g;
	for (int i = 0; i < 7; i++) g.addVertex();

	g[0].p = Point(0, 0, 0);
	for (int i = 1; i < 7; i++) g[0].addAdjacency(i);

	g[1].p = Point(1, 0, 1);
	g[2].p = Point(1, -1, -1);
	g[3].p = Point(-1, -1, 1);
	g[4].p = Point(-1, 0, -2);
	g[5].p = Point(-1, 1, 1);
	g[6].p = Point(1, 1, -1);
	for (int i = 1; i < 7; i++) g[i].addAdjacency(0);

	InputDcel dcel(g);
	dcel.splitMonkeySaddles();
	/*MsComplex msc(dcel);

	// this should produce three minima (2, 4, 6), and one saddle (0)
	REQUIRE(msc.vertexCount() == 4);

	// there should be three MS edges, from the saddle to all three minima
	REQUIRE(msc.halfEdgeCount() == 3 * 2);

	// only the outer face
	REQUIRE(msc.faceCount() == 1);*/
}

TEST_CASE("computePersistence()") {
	InputGraph g;
	for (int i = 0; i < 14; i++) g.addVertex();

	g[0].p =  Point(-2, -1, -1);
	g[1].p =  Point(-3,  0,  0);
	g[2].p =  Point(-2,  1,  0);
	g[3].p =  Point(-2,  0, 19);
	g[4].p =  Point(-1,  0,  9);
	g[5].p =  Point( 2, -1,  0);
	g[6].p =  Point( 3,  0, -1);
	g[7].p =  Point( 2,  1,  0);
	g[8].p =  Point( 2,  0, 20);
	g[9].p =  Point( 1,  0, 10);
	g[10].p = Point( 0, -1, -1);
	g[11].p = Point( 0,  0, 18);
	g[12].p = Point( 0,  1, -1);
	g[13].p = Point( 0,  0, std::numeric_limits<double>::infinity());

	g[0].addAdjacency(10);
	g[0].addAdjacency(13);
	g[0].addAdjacency(1);
	g[0].addAdjacency(3);

	g[1].addAdjacency(0);
	g[1].addAdjacency(13);
	g[1].addAdjacency(2);
	g[1].addAdjacency(3);

	g[2].addAdjacency(1);
	g[2].addAdjacency(13);
	g[2].addAdjacency(12);
	g[2].addAdjacency(3);

	g[3].addAdjacency(10);
	g[3].addAdjacency(0);
	g[3].addAdjacency(1);
	g[3].addAdjacency(2);
	g[3].addAdjacency(12);
	g[3].addAdjacency(4);

	g[4].addAdjacency(10);
	g[4].addAdjacency(3);
	g[4].addAdjacency(12);
	g[4].addAdjacency(11);

	g[5].addAdjacency(6);
	g[5].addAdjacency(13);
	g[5].addAdjacency(10);
	g[5].addAdjacency(8);

	g[6].addAdjacency(7);
	g[6].addAdjacency(13);
	g[6].addAdjacency(5);
	g[6].addAdjacency(8);

	g[7].addAdjacency(12);
	g[7].addAdjacency(13);
	g[7].addAdjacency(6);
	g[7].addAdjacency(8);

	g[8].addAdjacency(10);
	g[8].addAdjacency(9);
	g[8].addAdjacency(12);
	g[8].addAdjacency(7);
	g[8].addAdjacency(6);
	g[8].addAdjacency(5);

	g[9].addAdjacency(10);
	g[9].addAdjacency(11);
	g[9].addAdjacency(12);
	g[9].addAdjacency(8);

	g[10].addAdjacency(5);
	g[10].addAdjacency(13);
	g[10].addAdjacency(0);
	g[10].addAdjacency(3);
	g[10].addAdjacency(4);
	g[10].addAdjacency(11);
	g[10].addAdjacency(9);
	g[10].addAdjacency(8);

	g[11].addAdjacency(10);
	g[11].addAdjacency(4);
	g[11].addAdjacency(12);
	g[11].addAdjacency(9);

	g[12].addAdjacency(2);
	g[12].addAdjacency(13);
	g[12].addAdjacency(7);
	g[12].addAdjacency(8);
	g[12].addAdjacency(9);
	g[12].addAdjacency(11);
	g[12].addAdjacency(4);
	g[12].addAdjacency(3);

	g[13].addAdjacency(7);
	g[13].addAdjacency(6);
	g[13].addAdjacency(5);
	g[13].addAdjacency(10);
	g[13].addAdjacency(0);
	g[13].addAdjacency(1);
	g[13].addAdjacency(2);
	g[13].addAdjacency(12);

	InputDcel dcel(g);
	MsComplex msc;
	MsComplexCreator msCreator(dcel, &msc);
	msCreator.create();

	for (int i = 0; i < msc.faceCount(); i++) {
		MsComplex::Face f = msc.face(i);

		switch (f.data().maximum.id()) {
		case 3:
			REQUIRE(f.data().persistence == Approx(10));
			break;
		case 8:
			REQUIRE(f.data().persistence == Approx(20));
			break;
		case 11:
			REQUIRE(f.data().persistence == Approx(8));
			break;
		case 13:
			REQUIRE(f.data().persistence ==
			        std::numeric_limits<double>::infinity());
			break;
		}
	}
}

TEST_CASE("creating a Morse-Smale complex with non-square aspect ratio") {

	// create some heightmaps with the same height everywhere
	QImage image = QImage(4, 3, QImage::Format_RGB32);

	image.setPixel(0, 0, 2);
	image.setPixel(1, 0, 3);
	image.setPixel(2, 0, 5);
	image.setPixel(3, 0, 2);

	image.setPixel(0, 1, 5);
	image.setPixel(1, 1, 8);
	image.setPixel(2, 1, 4);
	image.setPixel(3, 1, 0);

	image.setPixel(0, 2, 7);
	image.setPixel(1, 2, 6);
	image.setPixel(2, 2, 1);
	image.setPixel(3, 2, 5);

	Units units;

	SECTION("square aspect ratio") {
		units = Units(1, 1, 0, 10);
	}

	SECTION("non-square aspect ratio") {
		units = Units(1, 2, 0, 10);
	}

	HeightMap heightMap(image);
	InputGraph g(heightMap, units);

	InputDcel dcel(g);
	dcel.splitMonkeySaddles();
	MsComplex msc;
	MsComplexCreator msCreator(dcel, &msc);
	msCreator.create();
}
