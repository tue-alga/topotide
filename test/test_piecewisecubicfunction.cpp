#include "catch.hpp"

#include <iostream>

#include "piecewisecubicfunction.h"
#include "point.h"

TEST_CASE("cubic functions") {
	CubicFunction f(1, 2, 3, 4);
	REQUIRE(f(0) == Approx(1));
	REQUIRE(f(1) == Approx(10));
	REQUIRE(f(2) == Approx(49));
	REQUIRE(f(3) == Approx(142));
}

TEST_CASE("volume function of a flat triangle") {
	Point p1(0, 0, 0);
	Point p2(0, 1, 0);
	Point p3(1, 0, 0);
	PiecewiseCubicFunction f(p1, p2, p3);

	REQUIRE(f(-2) == Approx(1));
	REQUIRE(f(-1) == Approx(0.5));
	REQUIRE(f(0) == Approx(0));
	REQUIRE(f(1) == Approx(0));
	REQUIRE(f(2) == Approx(0));
}

TEST_CASE("volume function of a non-flat triangle") {
	Point p1(0, 0, 10);
	Point p2(0, 1, 60);
	Point p3(10, 1, 190);
	PiecewiseCubicFunction f(p1, p2, p3);

	CHECK(f(0) == Approx(1300.0 / 3 - 5 * 0));
	CHECK(f(10) == Approx(1300.0 / 3 - 5 * 10));
	CHECK(f(60) == Approx(-(-190 + 60) * (-190 + 60) * (-190 + 60) / 14040.0));
	CHECK(f(190) == Approx(0));
}

TEST_CASE("valueAt") {
	PiecewiseCubicFunction f1({0}, {CubicFunction(1), CubicFunction(2)});
	CHECK(f1(-1) == Approx(1));
	CHECK(f1(1) == Approx(2));
	CHECK(f1(3) == Approx(2));

	PiecewiseCubicFunction f2({2}, {CubicFunction(3), CubicFunction(1)});
	CHECK(f2(-1) == Approx(3));
	CHECK(f2(1) == Approx(3));
	CHECK(f2(3) == Approx(1));
}

TEST_CASE("adding piecewise cubic functions") {
	PiecewiseCubicFunction f1({0}, {CubicFunction(1), CubicFunction(2)});
	PiecewiseCubicFunction f2({2}, {CubicFunction(3), CubicFunction(1)});
	PiecewiseCubicFunction f3 = f1.add(f2);

	CHECK(f3(-1) == Approx(4));
	CHECK(f3(1) == Approx(5));
	CHECK(f3(3) == Approx(3));
}
