#include "catch.hpp"

#include "../boundary.h"
#include "../path.h"

TEST_CASE("path spike removal") {

	Path p;
	REQUIRE(p.length() == -1);

	SECTION("simple path") {
		p.addPoint(HeightMap::Coordinate(2, 3));
		p.addPoint(HeightMap::Coordinate(6, 1));
		p.addPoint(HeightMap::Coordinate(2, 3));
		REQUIRE(p.length() == 2);
		p.removeSpikes();
		REQUIRE(p.length() == 0);
		REQUIRE(p.m_points[0] == HeightMap::Coordinate(2, 3));
	}

	SECTION("path with doubles") {
		p.addPoint(HeightMap::Coordinate(0, 0));
		p.addPoint(HeightMap::Coordinate(1, 0));
		p.addPoint(HeightMap::Coordinate(1, 0));
		p.addPoint(HeightMap::Coordinate(2, 0));
		REQUIRE(p.length() == 3);
		p.removeSpikes();
		REQUIRE(p.length() == 2);
		REQUIRE(p.m_points[0] == HeightMap::Coordinate(0, 0));
		REQUIRE(p.m_points[1] == HeightMap::Coordinate(1, 0));
		REQUIRE(p.m_points[2] == HeightMap::Coordinate(2, 0));
	}

	SECTION("spike with double in it") {
		p.addPoint(HeightMap::Coordinate(0, 0));
		p.addPoint(HeightMap::Coordinate(1, 0));
		p.addPoint(HeightMap::Coordinate(1, 0));
		p.addPoint(HeightMap::Coordinate(0, 0));
		REQUIRE(p.length() == 3);
		p.removeSpikes();
		REQUIRE(p.length() == 0);
		REQUIRE(p.m_points[0] == HeightMap::Coordinate(0, 0));
	}

	SECTION("deep spike") {
		p.addPoint(HeightMap::Coordinate(0, 0));
		p.addPoint(HeightMap::Coordinate(1, 0));
		p.addPoint(HeightMap::Coordinate(2, 0));
		p.addPoint(HeightMap::Coordinate(1, 0));
		p.addPoint(HeightMap::Coordinate(0, 0));
		p.addPoint(HeightMap::Coordinate(0, 1));
		REQUIRE(p.length() == 5);
		p.removeSpikes();
		REQUIRE(p.length() == 1);
		REQUIRE(p.m_points[0] == HeightMap::Coordinate(0, 0));
		REQUIRE(p.m_points[1] == HeightMap::Coordinate(0, 1));
	}

	SECTION("nested spike") {
		p.addPoint(HeightMap::Coordinate(0, 0));
		p.addPoint(HeightMap::Coordinate(1, 0));
		p.addPoint(HeightMap::Coordinate(0, 0));
		p.addPoint(HeightMap::Coordinate(1, 0));
		p.addPoint(HeightMap::Coordinate(0, 0));
		REQUIRE(p.length() == 4);
		p.removeSpikes();
		REQUIRE(p.length() == 0);
		REQUIRE(p.m_points[0] == HeightMap::Coordinate(0, 0));
	}
}

TEST_CASE("path rasterization") {
	Path p;
	REQUIRE(p.length() == -1);

	SECTION("one length-1 edge") {
		p.addPoint(HeightMap::Coordinate(0, 0));
		p.addPoint(HeightMap::Coordinate(1, 0));
		REQUIRE(p.length() == 1);
		Path rasterized = p.rasterize();
		REQUIRE(rasterized.length() == 1);
		REQUIRE(rasterized.m_points[0] == HeightMap::Coordinate(0, 0));
		REQUIRE(rasterized.m_points[1] == HeightMap::Coordinate(1, 0));
	}

	SECTION("several length-1 edges") {
		p.addPoint(HeightMap::Coordinate(0, 0));
		p.addPoint(HeightMap::Coordinate(1, 0));
		p.addPoint(HeightMap::Coordinate(1, 1));
		p.addPoint(HeightMap::Coordinate(1, 2));
		REQUIRE(p.length() == 3);
		Path rasterized = p.rasterize();
		REQUIRE(rasterized.length() == 3);
		REQUIRE(rasterized.m_points[0] == HeightMap::Coordinate(0, 0));
		REQUIRE(rasterized.m_points[1] == HeightMap::Coordinate(1, 0));
		REQUIRE(rasterized.m_points[2] == HeightMap::Coordinate(1, 1));
		REQUIRE(rasterized.m_points[3] == HeightMap::Coordinate(1, 2));
	}

	SECTION("one longer edge") {
		p.addPoint(HeightMap::Coordinate(0, 0));
		p.addPoint(HeightMap::Coordinate(2, 2));
		REQUIRE(p.length() == 1);
		Path rasterized = p.rasterize();
		REQUIRE(rasterized.length() == 4);
		REQUIRE(rasterized.m_points[0] == HeightMap::Coordinate(0, 0));
		REQUIRE(rasterized.m_points[2] == HeightMap::Coordinate(1, 1));
		REQUIRE(rasterized.m_points[4] == HeightMap::Coordinate(2, 2));
	}

	SECTION("one even longer edge") {
		p.addPoint(HeightMap::Coordinate(0, 0));
		p.addPoint(HeightMap::Coordinate(9, 12));
		REQUIRE(p.length() == 1);
		Path rasterized = p.rasterize();
		REQUIRE(rasterized.length() == 21);
		REQUIRE(rasterized.m_points[0] == HeightMap::Coordinate(0, 0));
		REQUIRE(rasterized.m_points[7] == HeightMap::Coordinate(3, 4));
		REQUIRE(rasterized.m_points[21] == HeightMap::Coordinate(9, 12));
	}

	SECTION("one longer horizontal edge") {
		p.addPoint(HeightMap::Coordinate(0, 0));
		p.addPoint(HeightMap::Coordinate(4, 0));
		REQUIRE(p.length() == 1);
		Path rasterized = p.rasterize();
		REQUIRE(rasterized.length() == 4);
		REQUIRE(rasterized.m_points[0] == HeightMap::Coordinate(0, 0));
		REQUIRE(rasterized.m_points[1] == HeightMap::Coordinate(1, 0));
		REQUIRE(rasterized.m_points[2] == HeightMap::Coordinate(2, 0));
		REQUIRE(rasterized.m_points[3] == HeightMap::Coordinate(3, 0));
		REQUIRE(rasterized.m_points[4] == HeightMap::Coordinate(4, 0));
	}
}
