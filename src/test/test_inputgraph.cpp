#include <QColor>
#include <QImage>
#include <iostream>

#include "catch.hpp"

#include "../inputgraph.h"
#include "../vertextype.h"

TEST_CASE("basic graph operations") {

	InputGraph g;
	REQUIRE(g.vertexCount() == 0);

	SECTION("adding vertices to the graph") {
		g.addVertex();
		REQUIRE(g.vertexCount() == 1);
		g.addVertex();
		REQUIRE(g.vertexCount() == 2);
	}
}

SCENARIO("creating a graph from a heightmap") {

	GIVEN("a 2x2 heightmap") {
		QImage image(2, 2, QImage::Format_RGB32);
		image.fill(QColor("black"));
		HeightMap heightMap(image);

		WHEN("converting it to a graph") {
			InputGraph g(heightMap);
			THEN("vertices should have correct degrees") {
				REQUIRE(g.vertexCount() == 7);
				REQUIRE(g[0].adj.size() == 3);
				REQUIRE(g[1].adj.size() == 3);
				REQUIRE(g[2].adj.size() == 6);
			}
		}
	}

	GIVEN("a 5x5 heightmap") {
		QImage image(5, 5, QImage::Format_RGB32);
		image.fill(QColor("black"));
		HeightMap heightMap(image);

		WHEN("converting it to a graph") {
			InputGraph g(heightMap);

			THEN("all vertices should have correct degrees") {
				REQUIRE(g.vertexCount() == 3 + 25);
				REQUIRE(g[0].adj.size() == 6);
				REQUIRE(g[1].adj.size() == 6);
				REQUIRE(g[2].adj.size() == 2 + 5 + 5);
				int expected[5][5] = {
				    {5, 5, 5, 5, 4},
				    {5, 6, 6, 6, 5},
				    {5, 6, 6, 6, 5},
				    {5, 6, 6, 6, 5},
				    {4, 5, 5, 5, 5}
				};
				for (int i = 3; i < g.vertexCount(); i++) {
					REQUIRE(g[i].adj.size() ==
					        expected[(int) g[i].p.y][(int) g[i].p.x]);
				}
			}

			THEN("heights should be set correctly") {
				REQUIRE(g[0].p.h == -std::numeric_limits<double>::infinity());
				REQUIRE(g[1].p.h == -std::numeric_limits<double>::infinity());
				REQUIRE(g[2].p.h == std::numeric_limits<double>::infinity());
				for (int i = 3; i < g.vertexCount(); i++) {
					REQUIRE(g[i].p.h == 0);
				}
			}
		}
	}
}

TEST_CASE("InputGraph::getWedges() and InputGraph::vertexType()") {
	InputGraph g;
	for (int i = 0; i < 5; i++) g.addVertex();
	g[0].p = Point(0, 0, 0);
	g[1].p = Point(1, 0, 0);
	g[2].p = Point(0, 1, 0);
	g[3].p = Point(-1, 0, 0);
	g[4].p = Point(0, -1, 0);
	g[0].addAdjacency(1);
	g[0].addAdjacency(2);
	g[0].addAdjacency(3);
	g[0].addAdjacency(4);

	SECTION("minimum") {
		g[0].p.h = -1;
		g[1].p.h = 1;
		InputGraph::Wedges w = g.getWedges(0);

		REQUIRE(w.ascending.size() == 1);
		REQUIRE(g[0].adj[w.ascending[0]].to == 1);
		REQUIRE(w.descending.size() == 0);

		REQUIRE(g.vertexType(0) == VertexType::minimum);
	}
	SECTION("maximum") {
		g[0].p.h = 1;
		g[2].p.h = -1;
		InputGraph::Wedges w = g.getWedges(0);

		REQUIRE(w.ascending.size() == 0);
		REQUIRE(w.descending.size() == 1);
		REQUIRE(g[0].adj[w.descending[0]].to == 2);

		REQUIRE(g.vertexType(0) == VertexType::maximum);
	}
	SECTION("regular") {
		g[0].p.h = 1;
		g[1].p.h = 2;
		g[2].p.h = 3;
		g[3].p.h = -1;
		InputGraph::Wedges w = g.getWedges(0);

		REQUIRE(w.ascending.size() == 1);
		REQUIRE(g[0].adj[w.ascending[0]].to == 2);
		REQUIRE(w.descending.size() == 1);
		REQUIRE(g[0].adj[w.descending[0]].to == 3);

		REQUIRE(g.vertexType(0) == VertexType::regular);
	}
	SECTION("saddle") {
		g[0].p.h = 1;
		g[1].p.h = 2;
		g[3].p.h = 2;
		REQUIRE(g.vertexType(0) == VertexType::saddle);
	}
}

TEST_CASE("InputGraph::getWedges() returns steepest-descent edges") {
	InputGraph g;
	for (int i = 0; i < 3; i++) g.addVertex();
	g[0].p = Point(0, 0, 0);
	g[1].p = Point(-1, 0, 1);
	g[2].p = Point(1, 0, 2);
	g[0].addAdjacency(1);
	g[0].addAdjacency(2);

	SECTION("equal-length edges") {
		g[2].p.x = 1;
		InputGraph::Wedges w = g.getWedges(0);
		REQUIRE(w.ascending.size() == 1);
		REQUIRE(w.descending.size() == 0);

		REQUIRE(g[0].adj[w.ascending[0]].to == 2);
	}

	SECTION("unequal-length edges") {
		g[2].p.x = 3;
		InputGraph::Wedges w = g.getWedges(0);
		REQUIRE(w.ascending.size() == 1);
		REQUIRE(w.descending.size() == 0);

		REQUIRE(g[0].adj[w.ascending[0]].to == 1);
	}
}
