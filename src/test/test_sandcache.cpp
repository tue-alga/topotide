#include "catch.hpp"

#include <iostream>
#include <vector>

#include <QColor>
#include <QImage>

#include "../network.h"
#include "../mscomplexcreator.h"
#include "../sandcache.h"
#include "../sortedpathscreator.h"
#include "../units.h"

TEST_CASE("test sand function computation for one face") {
	QImage image = QImage(2, 3, QImage::Format_RGB32);
	image.fill(QColor("black"));
	image.setPixel(0, 0, qRgb(0, 0, 1));
	image.setPixel(0, 1, qRgb(0, 0, 3));
	image.setPixel(0, 2, qRgb(0, 0, 2));
	image.setPixel(1, 0, qRgb(0, 0, 1));
	image.setPixel(1, 1, qRgb(0, 0, 3));
	image.setPixel(1, 2, qRgb(0, 0, 2));
	HeightMap heightMap(image);

	InputGraph g(heightMap);
	InputDcel dcel(g);
	MsComplex msc;
	MsComplexCreator msCreator(dcel, &msc);
	msCreator.create();

	MsComplex::Face f = msc.face(1);
	REQUIRE(f.data().maximum.data().p.h == Approx(3));
	REQUIRE(f.data().persistence == Approx(1));

	Striation striation;
	StriationCreator striationCreator(msc, &striation, Units(), false);
	striationCreator.create();

	std::vector<Network::Path> paths;
	SortedPathsCreator spCreator(&msc, &striation, &paths);
	spCreator.create();

	SandCache waterLevel(&msc, &striation,
	               &SandCache::waterLevelSandFunction);
	SandCache waterFlow(&msc, &striation,
	               &SandCache::waterFlowSandFunction);
	SandCache symmetricFlow(&msc, &striation,
	               &SandCache::symmetricFlowSandFunction);
	Network::Path topPath = paths[0];
	Network::Path bottomPath = paths[1];

	REQUIRE(waterLevel.sandFunction(topPath, bottomPath) ==
	        Approx(f.data().volumeAbove(1)));
	REQUIRE(waterFlow.sandFunction(topPath, bottomPath) ==
	        Approx(f.data().volumeAbove(1)));
	REQUIRE(symmetricFlow.sandFunction(topPath, bottomPath) ==
	        Approx(f.data().volumeAbove(2)));

	REQUIRE(waterLevel.sandFunction(topPath, bottomPath) ==
	        Approx(2.5));
	REQUIRE(waterFlow.sandFunction(topPath, bottomPath) ==
	        Approx(2.5));
	REQUIRE(symmetricFlow.sandFunction(topPath, bottomPath) ==
	        Approx(0.75));

	REQUIRE(waterLevel.sandFunction(bottomPath, topPath) ==
	        Approx(f.data().volumeAbove(2)));
	REQUIRE(waterFlow.sandFunction(bottomPath, topPath) ==
	        Approx(f.data().volumeAbove(2)));
	REQUIRE(symmetricFlow.sandFunction(bottomPath, topPath) ==
	        Approx(f.data().volumeAbove(2)));

	REQUIRE(waterLevel.sandFunction(bottomPath, topPath) ==
	        Approx(0.75));
	REQUIRE(waterFlow.sandFunction(bottomPath, topPath) ==
	        Approx(0.75));
	REQUIRE(symmetricFlow.sandFunction(bottomPath, topPath) ==
	        Approx(0.75));
}

TEST_CASE("test sand function computation for two faces") {
	QImage image = QImage(2, 5, QImage::Format_RGB32);
	image.fill(QColor("black"));
	for (int i = 0; i < 2; i++) {
		image.setPixel(i, 0, qRgb(0, 0, 1));
		image.setPixel(i, 1, qRgb(0, 0, 3));
		image.setPixel(i, 2, qRgb(0, 0, 2));
		image.setPixel(i, 3, qRgb(0, 0, 5));
		image.setPixel(i, 4, qRgb(0, 0, 4));
	}
	HeightMap heightMap(image);

	InputGraph g(heightMap);
	InputDcel dcel(g);
	MsComplex msc;
	MsComplexCreator msCreator(dcel, &msc);
	msCreator.create();

	REQUIRE(msc.faceCount() == 3);
	MsComplex::Face topFace;
	MsComplex::Face bottomFace;
	if (msc.face(1).data().maximum.data().p.y == 1) {
		topFace = msc.face(1);
		bottomFace = msc.face(2);
	} else {
		topFace = msc.face(2);
		bottomFace = msc.face(1);
	}
	REQUIRE(topFace.data().maximum.data().p.h == Approx(3));
	REQUIRE(bottomFace.data().maximum.data().p.h == Approx(5));
	REQUIRE(topFace.data().persistence == Approx(1));
	REQUIRE(bottomFace.data().persistence == Approx(1));

	Striation striation;
	StriationCreator striationCreator(msc, &striation, Units(), false);
	striationCreator.create();

	std::vector<Network::Path> paths;
	SortedPathsCreator spCreator(&msc, &striation, &paths);
	spCreator.create();

	SandCache waterLevel(&msc, &striation,
	               &SandCache::waterLevelSandFunction);
	SandCache waterFlow(&msc, &striation,
	               &SandCache::waterFlowSandFunction);
	SandCache symmetricFlow(&msc, &striation,
	               &SandCache::symmetricFlowSandFunction);
	Network::Path topPath = paths[0];
	Network::Path middlePath = paths[1];
	Network::Path bottomPath = paths[2];

	REQUIRE(waterLevel.sandFunction(topPath, bottomPath) ==
	        Approx(topFace.data().volumeAbove(1) +
	               bottomFace.data().volumeAbove(1)));
	REQUIRE(waterFlow.sandFunction(topPath, bottomPath) ==
	        Approx(topFace.data().volumeAbove(1) +
	               bottomFace.data().volumeAbove(1)));
	REQUIRE(symmetricFlow.sandFunction(topPath, bottomPath) ==
	        Approx(topFace.data().volumeAbove(2) +
	               bottomFace.data().volumeAbove(4)));

	REQUIRE(waterLevel.sandFunction(topPath, bottomPath) ==
	        Approx(8.5));
	REQUIRE(waterFlow.sandFunction(topPath, bottomPath) ==
	        Approx(8.5));
	REQUIRE(symmetricFlow.sandFunction(topPath, bottomPath) ==
	        Approx(17.0 / 12));

	REQUIRE(waterLevel.sandFunction(bottomPath, topPath) ==
	        Approx(bottomFace.data().volumeAbove(4) +
	               topFace.data().volumeAbove(4)));
	REQUIRE(waterFlow.sandFunction(bottomPath, topPath) ==
	        Approx(bottomFace.data().volumeAbove(4) +
	               topFace.data().volumeAbove(2)));
	REQUIRE(symmetricFlow.sandFunction(bottomPath, topPath) ==
	        Approx(bottomFace.data().volumeAbove(4) +
	               topFace.data().volumeAbove(2)));

	REQUIRE(waterLevel.sandFunction(bottomPath, topPath) ==
	        Approx(2.0 / 3));
	REQUIRE(waterFlow.sandFunction(bottomPath, topPath) ==
	        Approx(17.0 / 12));
	REQUIRE(symmetricFlow.sandFunction(bottomPath, topPath) ==
	        Approx(17.0 / 12));
}
