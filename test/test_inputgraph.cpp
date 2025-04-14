#include <QColor>
#include <QImage>
#include <limits>

#include "catch.hpp"

#include "inputgraph.h"

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
		HeightMap heightMap(2, 2);

		WHEN("converting it to a graph") {
			InputGraph g(heightMap);
			THEN("vertices should have correct degrees") {
				REQUIRE(g.vertexCount() == 6);
				REQUIRE(g[0].adj.size() == 2);
				REQUIRE(g[1].adj.size() == 2);
			}
		}
	}

	GIVEN("a 5x5 heightmap") {
		HeightMap heightMap(5, 5);
		for (int y = 0; y < 5; y++) {
			for (int x = 0; x < 5; x++) {
				heightMap.setElevationAt(x, y, 5 * y + x);
			}
		}

		WHEN("converting it to a graph") {
			InputGraph g(heightMap);

			THEN("all vertices should have correct degrees") {
				REQUIRE(g.vertexCount() == 2 + 25);
				REQUIRE(g[0].adj.size() == 5);
				REQUIRE(g[1].adj.size() == 5);
				for (int i = 2; i < g.vertexCount(); i++) {
					if (g[i].p.y == 0 || g[i].p.y == 4) {
						REQUIRE(g[i].adj.size() == 3);
					} else {
						REQUIRE(g[i].adj.size() == 4);
					}
				}
			}

			THEN("heights should be set correctly") {
				REQUIRE(g[0].p.h == -std::numeric_limits<double>::infinity());
				REQUIRE(g[1].p.h == -std::numeric_limits<double>::infinity());
				for (int i = 2; i < g.vertexCount(); i++) {
					REQUIRE(g[i].p.h == 5 * g[i].p.y + g[i].p.x);
				}
			}
		}
	}
}
