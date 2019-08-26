#include <QColor>
#include "catch.hpp"

#include <QImage>

#include "../inputdcel.h"

SCENARIO("creating a DCEL from an InputGraph") {

	GIVEN("a input graph containing just one point") {
		InputGraph g;
		g.addVertex();
		REQUIRE(g.vertexCount() == 1);

		WHEN("creating a DCEL out of it") {
			InputDcel dcel(g);
			THEN("it should have the right number of vertices / halfedges / faces") {
				REQUIRE(dcel.vertexCount() == 1);
				REQUIRE(dcel.halfEdgeCount() == 0);
				REQUIRE(dcel.faceCount() == 0);
			}
		}
	}

	GIVEN("a simple input graph") {
		InputGraph g;
		for (int i = 0; i < 6; i++) g.addVertex();
		REQUIRE(g.vertexCount() == 6);

		g[0].addAdjacency(2);
		g[0].addAdjacency(1);

		g[1].addAdjacency(0);
		g[1].addAdjacency(2);
		g[1].addAdjacency(4);
		g[1].addAdjacency(3);

		g[2].addAdjacency(4);
		g[2].addAdjacency(1);
		g[2].addAdjacency(0);

		g[3].addAdjacency(1);
		g[3].addAdjacency(4);
		g[3].addAdjacency(5);

		g[4].addAdjacency(5);
		g[4].addAdjacency(3);
		g[4].addAdjacency(1);
		g[4].addAdjacency(2);

		g[5].addAdjacency(3);
		g[5].addAdjacency(4);

		WHEN("creating a DCEL out of it") {
			InputDcel dcel(g);
			THEN("it should have the right number of vertices / halfedges / faces") {
				REQUIRE(dcel.vertexCount() == 6);
				REQUIRE(dcel.halfEdgeCount() == 18);
				REQUIRE(dcel.faceCount() == 5);
			}
		}
	}

	GIVEN("a graph created from a 2x2 heightmap") {
		QImage image(2, 2, QImage::Format_RGB32);
		image.fill(QColor("black"));
		HeightMap heightMap(image);

		InputGraph g(heightMap);
		WHEN("creating a DCEL out of it") {
			InputDcel dcel(g);
			THEN("it should have the right number of vertices / halfedges / faces") {
				REQUIRE(dcel.vertexCount() == 7);
				REQUIRE(dcel.halfEdgeCount() == 30);
				REQUIRE(dcel.faceCount() == 10);
			}
		}
	}
}

TEST_CASE("splitMonkeySaddles()") {
	InputGraph g;
	for (int i = 0; i <= 6; i++) g.addVertex();

	g[0].p = Point(0, 0, 1);
	for (int i = 1; i <= 6; i++) g[0].addAdjacency(i);

	g[1].p = Point( 1,  0,  0);
	g[2].p = Point( 1, -1,  2);
	g[3].p = Point( 0, -1,  0);
	g[4].p = Point(-1,  0,  2);
	g[5].p = Point(-1,  1, -1);
	g[6].p = Point( 0,  1,  2);

	g[1].addAdjacency(2);
	g[1].addAdjacency(0);
	g[1].addAdjacency(6);

	g[2].addAdjacency(3);
	g[2].addAdjacency(0);
	g[2].addAdjacency(1);

	g[3].addAdjacency(4);
	g[3].addAdjacency(0);
	g[3].addAdjacency(2);

	g[4].addAdjacency(5);
	g[4].addAdjacency(0);
	g[4].addAdjacency(3);

	g[5].addAdjacency(6);
	g[5].addAdjacency(0);
	g[5].addAdjacency(4);

	g[6].addAdjacency(1);
	g[6].addAdjacency(0);
	g[6].addAdjacency(5);

	InputDcel dcel(g);
	REQUIRE(dcel.vertexCount() == 7);
	REQUIRE(dcel.halfEdgeCount() == 12 * 2);

	SECTION("with a monkey saddle") {
		dcel.splitMonkeySaddles();
		REQUIRE(dcel.vertexCount() == 8);
		REQUIRE(dcel.halfEdgeCount() == 14 * 2);

		// should copy the data
		REQUIRE(dcel.vertex(7).data().p.x == dcel.vertex(0).data().p.x);
		REQUIRE(dcel.vertex(7).data().p.y == dcel.vertex(0).data().p.y);
		REQUIRE(dcel.vertex(7).data().p.h == dcel.vertex(0).data().p.h);

		// should change the steepest-descent mark to point to the new edge
		// instead of the old one
		REQUIRE(dcel.vertex(7).data().steepestDescentEdge != -1);
		REQUIRE(dcel.halfEdge(dcel.vertex(7).data().steepestDescentEdge).
				origin() == dcel.vertex(7));
	}

	SECTION("without a monkey saddle") {
		dcel.vertex(2).data().p.h = -1;
		dcel.splitMonkeySaddles();
		REQUIRE(dcel.vertexCount() == 7);
		REQUIRE(dcel.halfEdgeCount() == 12 * 2);
	}

	REQUIRE(dcel.isValid(true));
}

TEST_CASE("volumeAbove() and volumeBelow()") {
	InputGraph g;
	for (int i = 0; i < 3; i++) g.addVertex();

	g[0].addAdjacency(1);
	g[0].addAdjacency(2);
	g[1].addAdjacency(2);
	g[1].addAdjacency(0);
	g[2].addAdjacency(0);
	g[2].addAdjacency(1);

	g[0].p = Point(0, 1, 0);
	g[1].p = Point(1, 1, 0);
	g[2].p = Point(0, 0, 1);

	InputDcel dcel(g);
	REQUIRE(dcel.faceCount() == 2);  // outer and inner face

	InputDcel::Face innerFace =
	        dcel.vertex(0).outgoingTo(dcel.vertex(2)).incidentFace();

	PiecewiseCubicFunction above = dcel.volumeAboveFunction(innerFace);
	PiecewiseCubicFunction below = dcel.volumeBelowFunction(innerFace);
	REQUIRE(above(0) == Approx(1.0 / 6));
	REQUIRE(below(0) == Approx(0.0));
	REQUIRE(above(0.5) == Approx(1.0 / 48));
	REQUIRE(above(1) == Approx(0.0));
	REQUIRE(below(1) == Approx(2.0 / 6));
}
