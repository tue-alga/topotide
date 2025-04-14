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

		g[0].addAdjacencyAfter(2);
		g[0].addAdjacencyAfter(1);

		g[1].addAdjacencyAfter(0);
		g[1].addAdjacencyAfter(2);
		g[1].addAdjacencyAfter(4);
		g[1].addAdjacencyAfter(3);

		g[2].addAdjacencyAfter(4);
		g[2].addAdjacencyAfter(1);
		g[2].addAdjacencyAfter(0);

		g[3].addAdjacencyAfter(1);
		g[3].addAdjacencyAfter(4);
		g[3].addAdjacencyAfter(5);

		g[4].addAdjacencyAfter(5);
		g[4].addAdjacencyAfter(3);
		g[4].addAdjacencyAfter(1);
		g[4].addAdjacencyAfter(2);

		g[5].addAdjacencyAfter(3);
		g[5].addAdjacencyAfter(4);

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
				REQUIRE(dcel.vertexCount() == 6);
				REQUIRE(dcel.halfEdgeCount() == 16);
				REQUIRE(dcel.faceCount() == 4);
			}
		}
	}
}

SCENARIO("measuring volume above a face") {
	GIVEN("a DCEL created from a 2x2 heightmap") {
		HeightMap heightMap(2, 2);
		heightMap.setElevationAt(0, 0, 10.0);
		heightMap.setElevationAt(0, 1, 20.0);
		heightMap.setElevationAt(1, 0, 30.0);
		heightMap.setElevationAt(1, 1, 40.0);

		InputGraph g(heightMap);
		InputDcel dcel(g);

		WHEN("measuring the volume above the face") {
			InputDcel::Vertex v = dcel.vertexAt(0, 0);
			InputDcel::HalfEdge e = v.outgoingTo(dcel.vertexAt(1, 0));
			PiecewiseLinearFunction above = dcel.volumeAbove(e.incidentFace());

			THEN("it should return the correct volume") {
				CHECK(above(40.0) == Approx(0.0));
				CHECK(above(30.0) == Approx(0.25 * 10.0));
				CHECK(above(20.0) == Approx(0.25 * 20.0 + 0.25 * 10.0));
				CHECK(above(10.0) == Approx(0.25 * 30.0 + 0.25 * 20.0 + 0.25 * 10.0));
				CHECK(above(0.0) == Approx(0.25 * 40.0 + 0.25 * 30.0 + 0.25 * 20.0 + 0.25 * 10.0));
				CHECK(above(-10.0) == Approx(0.25 * 50.0 + 0.25 * 40.0 + 0.25 * 30.0 + 0.25 * 20.0));
			}
		}
	}
}
