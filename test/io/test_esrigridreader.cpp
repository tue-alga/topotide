#include "catch.hpp"

#include <QImage>
#include <QString>

#include "io/esrigridreader.h"
#include "units.h"

TEST_CASE("reading a correct Esri grid file") {
	HeightMap heightMap;
	QString error;
	Units units;

	SECTION("normal file") {
		heightMap = EsriGridReader::readGridFile("data/test/esri-grid-correct.ascii", error, units);
	}
	SECTION("comma as decimal separator") {
		heightMap = EsriGridReader::readGridFile("data/test/esri-grid-correct-with-comma.ascii", error, units);
	}
	CHECK(!heightMap.isEmpty());
	CHECK(error == "");
	CHECK(units.m_xResolution == 50.0);
	CHECK(units.m_yResolution == 50.0);
}

TEST_CASE("reading incorrect Esri grid files") {
	HeightMap heightMap;
	QString error = "";
	Units units;

	SECTION("missing ncols value") {
		heightMap = EsriGridReader::readGridFile(
		    "data/test/esri-grid-missing-ncols.ascii", error, units);
	}
	SECTION("missing nrows value") {
		heightMap = EsriGridReader::readGridFile(
		    "data/test/esri-grid-missing-nrows.ascii", error, units);
	}
	SECTION("missing cellsize value") {
		heightMap = EsriGridReader::readGridFile(
		    "data/test/esri-grid-missing-cellsize.ascii", error, units);
	}
	SECTION("missing NODATA_value value") {
		heightMap = EsriGridReader::readGridFile(
		    "data/test/esri-grid-missing-nodata.ascii", error, units);
	}
	SECTION("non-integral ncols value") {
		heightMap = EsriGridReader::readGridFile(
		    "data/test/esri-grid-non-integral-ncols.ascii", error, units);
	}
	SECTION("insufficient elevation values") {
		heightMap = EsriGridReader::readGridFile(
		    "data/test/esri-grid-insufficient-elevation-values.ascii", error, units);
	}
	SECTION("no elevation values") {
		heightMap = EsriGridReader::readGridFile(
		    "data/test/esri-grid-no-elevation-values.ascii", error, units);
	}
	SECTION("premature end of file") {
		heightMap = EsriGridReader::readGridFile(
		    "data/test/esri-grid-premature-eof.ascii", error, units);
	}
	SECTION("non-existing file") {
		heightMap = EsriGridReader::readGridFile(
		    "data/test/esri-grid-non-existing-file.ascii", error, units);
	}
	CHECK(heightMap.isEmpty());
	CHECK(error.size() > 0);
}
