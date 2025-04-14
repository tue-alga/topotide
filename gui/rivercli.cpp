#include "rivercli.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QImage>

#include <iostream>

#include "boundaryreader.h"
#include "io/esrigridreader.h"
#include "io/gdalreader.h"
#include "io/textfilereader.h"
#include "linksequence.h"
#include "mscomplexcreator.h"
#include "mscomplexsimplifier.h"
#include "mstonetworkgraphcreator.h"

#include "graphwriter.h"
#include "linksequencewriter.h"

int RiverCli::runComputation(const QStringList& args) {

	QCommandLineParser parser;
	parser.setApplicationDescription("An implementation of our "
									 "braided river algorithms.");
	parser.addHelpOption();
	parser.addVersionOption();

	QCommandLineOption xResOption(
				QStringList() << "xRes",
				"Sets the x-resolution of the river, in meters per pixel. "
				"[default: 1 for river images, or read from the river "
				"text file]",
				"resolution");
	parser.addOption(xResOption);

	QCommandLineOption yResOption(
				QStringList() << "yRes",
				"Sets the y-resolution of the river, in meters per pixel. "
				"[default: 1 for river images, or read from the river "
				"text file]",
				"resolution");
	parser.addOption(yResOption);

	QCommandLineOption linksOption(
				QStringList() << "links",
				"Output a link sequence instead of a text file describing "
				"the graph.");
	parser.addOption(linksOption);

	QCommandLineOption boundaryOption(
				QStringList() << "boundary",
				"Specifies a river boundary file to read. If this is not "
				"given, the entire extent of the river image is used. ",
				"filename");
	parser.addOption(boundaryOption);

	parser.addPositionalArgument("input",
								 "The input river dataset.",
								 "<input>");

	parser.addPositionalArgument("output",
								 "The output network file. `.txt` or "
								 "`.ipe` is "
								 "appended automatically. If more than "
								 "one δ-value is given, the output files "
								 "are suffixed with the corresponding "
								 "δ-values.",
								 "<output>");

	parser.process(args);

	if (parser.positionalArguments().size() != 2) {
		std::cerr << "One input and one output argument required.\n";
		return 1;
	}

	Units units;

	QString inputFile = parser.positionalArguments()[0];
	HeightMap heightMap;
	QString error = "[no error given]";
	if (inputFile.endsWith(".txt")) {
		heightMap = TextFileReader::readTextFile(inputFile, error, units);
	} else if (inputFile.endsWith(".ascii") || inputFile.endsWith(".asc")) {
		heightMap = EsriGridReader::readGridFile(inputFile, error, units);
	} else {
		heightMap = GdalReader::readGdalFile(inputFile, error, units);
	}
	if (heightMap.isEmpty()) {
		std::cerr << "Could not read image or text file \""
				  << inputFile.toStdString() << "\".\n";
		std::cerr << "Reading the text file failed due to the following "
				  << "error: " << error.toStdString() << "\n";
		return 1;
	}

	QString output = parser.positionalArguments()[1];

	if (parser.isSet(xResOption)) {
		QString value = parser.value(xResOption);
		bool ok = false;
		double xRes = value.toDouble(&ok);
		if (!ok) {
			std::cerr << "x-resolution (--xRes) \""
					  << value.toStdString()
					  << "\" must be a number.\n";
			return 1;
		}
		units.m_xResolution = xRes;
	}

	if (parser.isSet(yResOption)) {
		QString value = parser.value(yResOption);
		bool ok = false;
		double yRes = value.toDouble(&ok);
		if (!ok) {
			std::cerr << "y-resolution (--yRes) \""
					  << value.toStdString()
					  << "\" must be a number.\n";
			return 1;
		}
		units.m_yResolution = yRes;
	}

	// command-line arguments are OK, let's run the algorithm

	Boundary boundary(heightMap);
	if (parser.isSet(boundaryOption)) {
		QString boundaryError = "";
		boundary = BoundaryReader::readBoundary(
					   parser.value(boundaryOption),
					   heightMap.width(), heightMap.height(),
					   boundaryError);
		if (boundaryError != "") {
			std::cerr << "Reading the river boundary file failed "
					  << "due to the following error: "
					  << boundaryError.toStdString() << "\n";
			return 1;
		}
	}

	if (!boundary.rasterize().isValid()) {
		std::cerr << "The computation cannot run as the boundary is invalid. A valid "
		             "boundary does not self-intersect and does not visit "
		             "any points more than once.\n";
		return 1;
	}

	std::cerr << "Computing input graph...\n";
	InputGraph inputGraph(heightMap, boundary);

	if (inputGraph.containsNodata()) {
		std::cerr << "The computation cannot run as there are nodata values inside the boundary.\n";
		return 1;
	}

	std::cerr << "Computing input DCEL...\n";
	auto inputDcel = std::make_shared<InputDcel>(inputGraph);
	inputDcel->computeGradientFlow();

	std::cerr << "Computing MS complex...     ";
	auto msComplex = std::make_shared<MsComplex>();
	MsComplexCreator msCreator(inputDcel, msComplex, [](int p) {
		std::cerr << "\b\b\b\b";
		std::cerr << std::setw(3) << p << "%";
	});
	msCreator.create();
	std::cerr << "\n";

	auto networkGraph = std::make_shared<NetworkGraph>();

	std::cerr << "Simplifying MS complex...     ";
	auto msSimplified = std::make_shared<MsComplex>(*msComplex);
	MsComplexSimplifier msSimplifier(
				msSimplified,
				[](int p) {
		std::cerr << "\b\b\b\b";
		std::cerr << std::setw(3) << p << "%";
	});
	msSimplifier.simplify();
	std::cerr << "\n";

	std::cerr << "Compacting MS complex...\n";
	msSimplified->compact();

	std::cerr << "Converting MS complex into network...     ";
	MsToNetworkGraphCreator networkGraphCreator(
				msSimplified, networkGraph,
				[](int p) {
		std::cerr << "\b\b\b\b";
		std::cerr << std::setw(3) << p << "%";
	});
	networkGraphCreator.create();
	std::cerr << "\n";

	std::cerr << "Writing graph...\n";
	if (parser.isSet(linksOption)) {
		LinkSequence links(*networkGraph);
		LinkSequenceWriter::writeLinkSequence(
					links,
					units,
					output + ".txt");
	} else {
		GraphWriter::writeGraph(*networkGraph,
								units,
								output + ".txt");
	}

	return 0;
}
