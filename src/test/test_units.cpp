#include "catch.hpp"

#include <cmath>

#include "../units.h"

TEST_CASE("default unit converter") {
	Units u;
	REQUIRE(u.length(1, 1, 2, 2) == std::sqrt(2));
	REQUIRE(u.toRealElevation(0x000000) == Approx(0));
	REQUIRE(u.toRealElevation(0x800000) == Approx(5));
	REQUIRE(u.toRealElevation(0xffffff) == Approx(10));
}

TEST_CASE("custom unit converter") {
	Units u(1, 2, 10, 20);
	REQUIRE(u.length(1, 1, 2, 2) == std::sqrt(5));
	REQUIRE(u.toRealElevation(0x000000) == Approx(10));
	REQUIRE(u.toRealElevation(0x800000) == Approx(15));
	REQUIRE(u.toRealElevation(0xffffff) == Approx(20));
}

TEST_CASE("volume conversion") {
	Units u(10, 5, 5, 15);
	REQUIRE(u.toRealVolume(1e8) == Approx(2980.232));
	REQUIRE(u.fromRealVolume(2980.232) == Approx(1e8));
}
