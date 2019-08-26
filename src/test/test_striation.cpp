#include <QColor>
#include <QImage>
#include <QRgb>

#include "catch.hpp"

#include "../inputgraph.h"
#include "../inputdcel.h"
#include "../mscomplex.h"
#include "../mscomplexcreator.h"
#include "../striation.h"
#include "../striationcreator.h"

TEST_CASE("creating a striation from a simple MS-complex") {
	QImage image = QImage(3, 3, QImage::Format_RGB32);
	image.fill(QColor("black"));
	image.setPixel(1, 1, qRgb(1, 0, 0));
	HeightMap heightMap(image);

	InputGraph g(heightMap);
	InputDcel dcel(g);
	MsComplex msc;
	MsComplexCreator msCreator(dcel, &msc);
	msCreator.create();

	Striation striation;
	StriationCreator striationCreator(msc, &striation, Units(), false);
	striationCreator.create();

	// there is only one maximum
	REQUIRE(striation.itemCount() == 1);
}

TEST_CASE("creating a striation where a carving path visits the source twice") {
	QImage image = QImage(3, 3, QImage::Format_RGB32);
	image.fill(QColor("black"));
	image.setPixel(1, 0, qRgb(20, 0, 0));
	image.setPixel(0, 1, qRgb(10, 0, 0));
	image.setPixel(2, 2, qRgb(30, 0, 0));
	HeightMap heightMap(image);

	InputGraph g(heightMap);
	InputDcel dcel(g);
	MsComplex msc;
	MsComplexCreator msCreator(dcel, &msc);
	msCreator.create();

	Striation striation;
	StriationCreator striationCreator(msc, &striation, Units(), false);
	striationCreator.create();

	// there is only one maximum (note that (0, 1) and (2, 2) are adjacent to
	// the global maximum, and hence are not actually maxima)
	REQUIRE(striation.itemCount() == 1);
}

TEST_CASE("creating a striation on a larger river") {
	QImage image = QImage(10, 10, QImage::Format_RGB32);
	image.fill(QColor("black"));
	image.setPixel(3, 7, qRgb(0, 0, 30));
	image.setPixel(5, 5, qRgb(0, 0, 20));
	image.setPixel(7, 3, qRgb(0, 0, 10));
	HeightMap heightMap(image);

	InputGraph g(heightMap);
	InputDcel dcel(g);
	MsComplex msc;
	MsComplexCreator msCreator(dcel, &msc);
	msCreator.create();

	Striation striation;
	StriationCreator striationCreator(msc, &striation, Units(), false);
	striationCreator.create();

	// there are three maxima
	REQUIRE(striation.itemCount() == 3);

	// first carve around (3, 7) - the highest-persistence maximum
	Striation::Item& item = striation.item(0);
	REQUIRE(msc.face(item.m_face).data().maximum.data().p.h ==
	        Approx(30));
	REQUIRE(item.m_topCarvePath.size() == 2);
	REQUIRE(item.m_bottomCarvePath.size() == 4);
	REQUIRE(item.m_topVertices.size() == 1);
	REQUIRE(item.m_bottomVertices.size() == 3);

	// there is no top item, as the lowest source-sink path passes the maxima
	// from the top side
	REQUIRE(item.m_topItem == -1);

	// look at the bottom item - second carve around (5, 5)
	item = striation.item(item.m_bottomItem);
	REQUIRE(msc.face(item.m_face).data().maximum.data().p.h ==
	        Approx(20));
	REQUIRE(item.m_topCarvePath.size() == 4);
	REQUIRE(item.m_bottomCarvePath.size() == 6);
	REQUIRE(item.m_topVertices.size() == 1);
	REQUIRE(item.m_bottomVertices.size() == 3);

	// there is no top item, as the lowest source-sink path passes the maxima
	// from the top side
	REQUIRE(item.m_topItem == -1);

	// look at the bottom item - third carve around (7, 3)
	item = striation.item(item.m_bottomItem);
	REQUIRE(msc.face(item.m_face).data().maximum.data().p.h ==
	        Approx(10));
	REQUIRE(item.m_topCarvePath.size() == 6);
	REQUIRE(item.m_bottomCarvePath.size() == 8);
	REQUIRE(item.m_topVertices.size() == 1);
	REQUIRE(item.m_bottomVertices.size() == 3);

	// now there is no top or bottom item left
	REQUIRE(item.m_topItem == -1);
	REQUIRE(item.m_bottomItem == -1);
}

TEST_CASE("carving should not leave parts of the striation "
          "unreachable", "[!mayfail]") {
	QImage image = QImage(10, 10, QImage::Format_RGB32);
	image.fill(QColor("black"));
	image.setPixel(3, 3, qRgb(10, 0, 0));
	image.setPixel(5, 5, qRgb(30, 0, 0));
	image.setPixel(7, 7, qRgb(30, 0, 0));
	HeightMap heightMap(image);

	InputGraph g(heightMap);
	InputDcel dcel(g);
	MsComplex msc;
	MsComplexCreator msCreator(dcel, &msc);
	msCreator.create();

	Striation striation;
	StriationCreator striationCreator(msc, &striation, Units(), false);
	striationCreator.create();

	// there are three maxima
	REQUIRE(striation.itemCount() == 3);
}
