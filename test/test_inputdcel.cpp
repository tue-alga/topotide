#include <QColor>
#include "catch.hpp"

#include <QImage>

#include "inputdcel.h"

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
		HeightMap heightMap(2, 2);

		InputGraph g(heightMap);
		WHEN("creating a DCEL out of it") {
			InputDcel dcel(g);
			THEN("it should have the right number of vertices / halfedges / faces") {
				REQUIRE(dcel.vertexCount() == 7);
				REQUIRE(dcel.halfEdgeCount() == 28);
				REQUIRE(dcel.faceCount() == 9);
			}
		}
	}
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
