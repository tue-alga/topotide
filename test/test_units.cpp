#include "catch.hpp"

#include <cmath>

#include "units.h"

TEST_CASE("volume conversion") {
	Units u(10, 5);
	REQUIRE(u.toRealVolume(10.0) == Approx(10.0 * 10 * 5));
	REQUIRE(u.fromRealVolume(10.0) == Approx(10.0 / (10 * 5)));
}
