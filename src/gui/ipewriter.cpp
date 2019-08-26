#ifndef WITH_IPELIB
#include <stdexcept>
#else
#include <iostream>

#include <QFile>
#include <QTemporaryFile>

#include <ipedoc.h>
#include <ipeiml.h>
#endif

#include "ipewriter.h"

#ifdef WITH_IPELIB
using namespace ipe;

int IpeWriter::m_height = 0;
double IpeWriter::m_verticalStretch = 1;
double IpeWriter::scaleFactor = 1;
#endif

void IpeWriter::writeIpeFile(const HeightMap& heightMap,
                             const NetworkGraph& graph,
                             const Units& units,
                             const QString& fileName) {

#ifndef WITH_IPELIB
	throw std::logic_error("Tried to use IpeWriter while Ipe support is unavailable");
#else

	Platform::initLib(IPELIB_VERSION);

	Document doc;

	// set page size
	m_height = heightMap.height();
	m_verticalStretch = units.m_yResolution / units.m_xResolution;
	Vector mapSize(heightMap.width() - 1, (m_height - 1) * m_verticalStretch);
	mapSize *= scaleFactor;

	Layout* layout = new Layout();
	Rect paperSize(Vector(0, 0), mapSize);
	layout->iOrigin = -paperSize.bottomLeft();
	layout->iPaperSize = Vector(paperSize.width(), paperSize.height());
	layout->iFrameSize = layout->iPaperSize;

	QFile styleFile(":/res/ipe/style.xml");
	styleFile.open(QFile::ReadOnly);
	QByteArray styleArray = styleFile.readAll();
	Buffer styleBuffer(styleArray.data(), styleArray.size());
	BufferSource styleSource(styleBuffer);
	ImlParser styleParser(styleSource);
	StyleSheet* styleSheet = styleParser.parseStyleSheet();
	if (!styleSheet) {
		std::cerr << "Stylesheet parse failure" << std::endl;
	}
	doc.cascade()->insert(0, styleSheet);

	StyleSheet* sheet = new StyleSheet();
	sheet->setName("paper-size");
	sheet->setLayout(*layout);
	doc.cascade()->insert(1, sheet);

	Page* page = new Page();
	page->addLayer("map");
	page->addLayer("network");

	// background map
	QTemporaryFile mapImage("river-map-XXXXXX.png");
	if (!mapImage.open()) {
		std::cerr << "Couldn't open temporary file" << std::endl;
		return;
	}
	QString mapImageFileName = mapImage.fileName();
	heightMap.image().save(mapImageFileName);
	QByteArray ba = mapImageFileName.toLocal8Bit();
	const char *mapImageFileName2 = ba.data();
	Vector dpi;
	const char* error;
	Bitmap mapBitmap = Bitmap::readPNG(mapImageFileName2, dpi, error);
	if (mapBitmap.isNull()) {
		std::cerr << "Error: Ipe couldn't read our image: "
		          << error << std::endl;
	} else {
		Image* map = new Image(Rect(Vector(0, 0), mapSize), mapBitmap);
		page->append(TSelect::ENotSelected, 0, map);
	}

	// output the link sequence
	LinkSequence links(graph);
	double deltaMax = links.linkCount() > 1 ? links.link(1).delta : 1;
	for (int i = links.linkCount() - 1; i >= 0; i--) {
		LinkSequence::Link& link = links.link(i);
		Path* path = linkToIpePath(link, deltaMax);
		page->append(TSelect::ENotSelected, 1, path);
	}

	doc.push_back(page);
	doc.save(fileName.toLocal8Bit(), FileFormat::Xml, 0);
#endif
}

#ifdef WITH_IPELIB
Path*
IpeWriter::linkToIpePath(const LinkSequence::Link& link, double deltaMax) {

	Curve* curve = new Curve();
	for (int i = 1; i < link.path.size(); i++) {
		double x1 = link.path[i - 1].x * scaleFactor;
		double y1 = m_verticalStretch * scaleFactor *
		            (m_height - 1 - link.path[i - 1].y);
		double x2 = link.path[i].x * scaleFactor;
		double y2 = m_verticalStretch * scaleFactor *
		            (m_height - 1 - link.path[i].y);
		curve->appendSegment(Vector(x1, y1), Vector(x2, y2));
	}

	Shape* shape = new Shape();
	shape->appendSubPath(curve);

	double delta = link.delta;
	QColor color;
	if (delta == std::numeric_limits<double>::infinity()) {
		color = QColor("#c7e9b4");
	} else if (delta > deltaMax / 1e1) {
		color = QColor("#7fcdbb");
	} else if (delta > deltaMax / 1e2) {
		color = QColor("#41b6c4");
	} else if (delta > deltaMax / 1e3) {
		color = QColor("#1d91c0");
	} else if (delta > deltaMax / 1e4) {
		color = QColor("#225ea8");
	} else if (delta > deltaMax / 1e5) {
		color = QColor("#253494");
	} else {
		color = QColor("#081d58");
	}
	double width = 4;
	if (delta < std::numeric_limits<double>::infinity()) {
		width = std::max(1.5, 3 - 0.5 * log10(deltaMax / delta));
	}

	AllAttributes attributes;
	attributes.iPen = Attribute(Fixed::fromDouble(width));
	attributes.iStroke = Attribute(toIpeColor(color));
	return new Path(attributes, *shape);
}

ipe::Color IpeWriter::toIpeColor(QColor color) {
	return Color(color.redF() * 1000,
	             color.greenF() * 1000,
	             color.blueF() * 1000);
}
#endif
