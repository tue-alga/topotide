#include "catch.hpp"

#include "piecewiselinearfunction.h"

TEST_CASE("linear functions") {
	LinearFunction f(1, 2);
	REQUIRE(f(0) == Approx(1));
	REQUIRE(f(1) == Approx(3));
	REQUIRE(f(2) == Approx(5));
	REQUIRE(f(3) == Approx(7));
}

TEST_CASE("valueAt") {
	PiecewiseLinearFunction f1({0}, {LinearFunction(1), LinearFunction(2)});
	CHECK(f1(-1) == Approx(1));
	CHECK(f1(1) == Approx(2));
	CHECK(f1(3) == Approx(2));

	PiecewiseLinearFunction f2({2}, {LinearFunction(3), LinearFunction(1)});
	CHECK(f2(-1) == Approx(3));
	CHECK(f2(1) == Approx(3));
	CHECK(f2(3) == Approx(1));
}

TEST_CASE("adding piecewise linear functions") {
	PiecewiseLinearFunction f1({0}, {LinearFunction(1), LinearFunction(2)});
	PiecewiseLinearFunction f2({2}, {LinearFunction(3), LinearFunction(1)});
	PiecewiseLinearFunction f3 = f1.add(f2);

	CHECK(f3(-1) == Approx(4));
	CHECK(f3(1) == Approx(5));
	CHECK(f3(3) == Approx(3));
}

TEST_CASE("searching heights for a given piecewise linear function value") {
	PiecewiseLinearFunction f1(
	    {-1, 1}, {LinearFunction(-1, -2), LinearFunction(0, -1), LinearFunction(-1, -0.5)});

	CHECK(f1.heightForVolume(3) == Approx(-2));
	CHECK(f1.heightForVolume(2.5) == Approx(-1.75));
	CHECK(f1.heightForVolume(2) == Approx(-1.5));
	CHECK(f1.heightForVolume(1.5) == Approx(-1.25));
	CHECK(f1.heightForVolume(1) == Approx(-1));
	CHECK(f1.heightForVolume(0.5) == Approx(-0.5));
	CHECK(f1.heightForVolume(0) == Approx(0));
	CHECK(f1.heightForVolume(-0.5) == Approx(0.5));
	CHECK(f1.heightForVolume(-1) == Approx(1));
	CHECK(f1.heightForVolume(-1.5) == Approx(1));
	CHECK(f1.heightForVolume(-2) == Approx(2));
	CHECK(f1.heightForVolume(-2.5) == Approx(3));
	CHECK(f1.heightForVolume(-3) == Approx(4));
}
