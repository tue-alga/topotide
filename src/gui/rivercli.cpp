#include "rivercli.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QFile>
#include <QImage>

#include <iostream>

#include "../boundaryreader.h"
#include "../linksequence.h"
#include "../mscomplexcreator.h"
#include "../mscomplexsimplifier.h"
#include "../mstonetworkgraphcreator.h"
#include "../networkcreator.h"
#include "../networkgraphcreator.h"
#include "../sortedpathscreator.h"
#include "../striationcreator.h"
#include "../textfilereader.h"

#include "graphwriter.h"
#include "ipewriter.h"
#include "linksequencewriter.h"

int RiverCli::runComputation(const QStringList& args) {

	QCommandLineParser parser;
	parser.setApplicationDescription("An implementation of our "
									 "braided river algorithms.");
	parser.addHelpOption();
	parser.addVersionOption();

	QCommandLineOption algorithmOption(
				QStringList() << "a" << "algorithm",
				"Selects the algorithm to use. Valid options are "
				"`striation` (the striation-based algorithm used in our "
				"SoCG paper) and `persistence` (the new persistence-based "
				"technique). [default: striation]",
				"alg");
	parser.addOption(algorithmOption);

	QCommandLineOption deltaOption(
				QStringList() << "d" << "delta",
				"A list of δ-values, in m³, separated by semicolons. "
				"This can only be used with the `striation` algorithm. "
				"Scientific notiation can be used, e.g. "
				"`--delta 100;1e3;1e4`. [default: 100]",
				"δ;δ;...");
	parser.addOption(deltaOption);

	QCommandLineOption deltaInternalOption(
				QStringList() << "deltaInternalUnits",
				"Causes the --delta option to be interpreted in internal "
				"units instead of m³. This was the default behaviour of "
				"older versions of this program (up until 1.2.0). This "
				"option is provided for compatibility only, and is not "
				"recommended.");
	parser.addOption(deltaInternalOption);

	QCommandLineOption bidirectionalOption(
				QStringList() << "b" << "bidirectional",
				"Use the bidirectional sand function. "
				"This can only be used with the `striation` algorithm.");
	parser.addOption(bidirectionalOption);

	QCommandLineOption simplifyOption(
				QStringList() << "s" << "simplify",
				"Simplify the output graph by removing "
				"the degree-2 vertices. "
				"This can only be used with the `striation` algorithm.");
	parser.addOption(simplifyOption);

	QCommandLineOption hybridStriationOption(
				QStringList() << "hybridStriation",
				"Use the hybrid striation strategy instead of the highest "
				"persistence first strategy.");
	parser.addOption(hybridStriationOption);

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

	QCommandLineOption minHeightOption(
				QStringList() << "minHeight",
				"Sets the minimum elevation of the river, in meters. "
				"[default: 0 for river images, or read from the river "
				"text file]",
				"elevation");
	parser.addOption(minHeightOption);

	QCommandLineOption maxHeightOption(
				QStringList() << "maxHeight",
				"Sets the maximum elevation of the river, in meters. "
				"[default: 10 for river images, or read from the river "
				"text file]",
				"elevation");
	parser.addOption(maxHeightOption);

	QCommandLineOption ipeOption(
				QStringList() << "ipe",
				"Output an Ipe figure instead of a text file describing "
				"the graph.");
	parser.addOption(ipeOption);

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
								 "The input river image or text file.",
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
	// is it an image?
	QString error = "[no error given]";
	QImage input(inputFile);
	if (input.width() == 0 || input.height() == 0) {
		// is it a text file?
		input = TextFileReader::readTextFile(inputFile, error, units);
	}
	if (input.isNull()) {
		std::cerr << "Could not read image or text file \""
				  << inputFile.toStdString() << "\".\n";
		std::cerr << "Reading the text file failed due to the following "
				  << "error: " << error.toStdString() << "\n";
		return 1;
	}

	QString output = parser.positionalArguments()[1];

	bool algorithm = false;  // false: striation; true: persistence

	if (parser.isSet(algorithmOption)) {
		QString algorithmValue = parser.value(algorithmOption);
		if (algorithmValue == "persistence") {
			algorithm = true;
		} else if (algorithmValue != "striation") {
			std::cerr << "Invalid algorithm name (available algorithms: "
						 "`persistence` or `striation`).\n";
			return 1;
		}
	}

	std::vector<double> deltas;
	// maintain the strings too, for progress messages
	QStringList deltaStrings;

	if (parser.isSet(deltaOption)) {
		if (algorithm) {
			std::cerr << "The --delta option cannot be used with the "
						 "`persistence` algorithm.\n";
			return 1;
		}
		QString deltaValue = parser.value(deltaOption);
		deltaStrings = deltaValue.split(";");
		for (const QString& value : deltaStrings) {
			bool ok = false;
			deltas.push_back(value.toDouble(&ok));
			if (!ok) {
				std::cerr << "δ-value (--delta) \""
						  << value.toStdString()
						  << "\" must be a number.\n";
				return 1;
			}
		}
	} else {
		deltas.push_back(100);
		deltaStrings.push_back("100");
	}

	if (parser.isSet(bidirectionalOption) && algorithm) {
		std::cerr << "The --bidirectional option cannot be used with the "
					 "`persistence` algorithm.\n";
		return 1;
	}

	if (parser.isSet(simplifyOption) && algorithm) {
		std::cerr << "The --simplify option cannot be used with the "
					 "`persistence` algorithm.\n";
		return 1;
	}

	if (parser.isSet(deltaInternalOption) && !parser.isSet(deltaOption)) {
		std::cerr << "The --deltaInternal option cannot be used without "
					 "the --delta option.\n";
		return 1;
	}

#ifndef WITH_IPELIB
	if (parser.isSet(ipeOption)) {
		std::cerr << "The --ipe option cannot be used because this program has "
		             "not been compiled with Ipelib support.\n";
		return 1;
	}
#endif

	if (parser.isSet(ipeOption) && parser.isSet(linksOption)) {
		std::cerr << "The --ipe and --links options cannot be used "
					 "at the same time.\n";
		return 1;
	}

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

	if (parser.isSet(minHeightOption)) {
		QString value = parser.value(minHeightOption);
		bool ok = false;
		double minHeight = value.toDouble(&ok);
		if (!ok) {
			std::cerr << "Minimum elevation (--minHeight) \""
					  << value.toStdString()
					  << "\" must be a number.\n";
			return 1;
		}
		units.m_minElevation = minHeight;
	}

	if (parser.isSet(maxHeightOption)) {
		QString value = parser.value(maxHeightOption);
		bool ok = false;
		double maxHeight = value.toDouble(&ok);
		if (!ok) {
			std::cerr << "Maximum elevation (--maxHeight) \""
					  << value.toStdString()
					  << "\" must be a number.\n";
			return 1;
		}
		units.m_maxElevation = maxHeight;
	}

	// convert deltas to internal units (unless user specified that they
	// are already in internal units)
	if (!parser.isSet(deltaInternalOption)) {
		for (double& delta : deltas) {
			delta = units.fromRealVolume(delta);
		}
	}

	// command-line arguments are OK, let's run the algorithm

	HeightMap heightMap(input);
	Boundary boundary(heightMap);
	if (parser.isSet(boundaryOption)) {
		QString boundaryError = "";
		boundary = BoundaryReader::readBoundary(
					   parser.value(boundaryOption),
					   heightMap,
					   boundaryError);
		if (boundaryError != "") {
			std::cerr << "Reading the river boundary file failed "
					  << "due to the following error: "
					  << boundaryError.toStdString() << "\n";
			return 1;
		}
	}

	std::cerr << "Computing input graph...\n";
	InputGraph inputGraph(heightMap,
	                      boundary,
	                      units);

	std::cerr << "Computing input DCEL...\n";
	InputDcel inputDcel(inputGraph);
	inputDcel.splitMonkeySaddles();

	std::cerr << "Computing MS complex...     ";
	MsComplex msComplex;
	MsComplexCreator msCreator(inputDcel, &msComplex, [](int p) {
		std::cerr << "\b\b\b\b";
		std::cerr << std::setw(3) << p << "%";
	});
	msCreator.create();
	std::cerr << "\n";

	Striation striation;
	std::vector<Network::Path> sortedPaths;

	if (!algorithm) {
		std::cerr << "Computing striation...     ";
		StriationCreator striationCreator(
					msComplex, &striation,
					units, parser.isSet(hybridStriationOption),
					[](int p) {
			std::cerr << "\b\b\b\b";
			std::cerr << std::setw(3) << p << "%";
		});
		striationCreator.create();
		std::cerr << "\n";

		std::cerr << "Sorting striation paths on height...     ";
		SortedPathsCreator sortedPathsCreator(
					&msComplex, &striation, &sortedPaths, [](int p) {
			std::cerr << "\b\b\b\b";
			std::cerr << std::setw(3) << p << "%";
		});
		sortedPathsCreator.create();
		std::cerr << "\n";
	}

	std::cerr << "Initializing sand cache...\n";
	SandCache sandCache(&msComplex, &striation,
						&SandCache::waterFlowSandFunction);

	for (int i = 0; i < deltas.size(); i++) {
		double delta = deltas[i];
		QString deltaString = deltaStrings[i];

		std::string prefix = "";
		if (deltas.size() > 1) {
			std::cerr << "δ = "
					  << deltaString.toStdString()
					  << ":\n";
			prefix = "    ";
		}

		NetworkGraph networkGraph;

		if (algorithm) {
			std::cerr << prefix << "Simplifying MS complex...     ";
			MsComplex msSimplified(msComplex);
			MsComplexSimplifier msSimplifier(
						msSimplified,
						[](int p) {
				std::cerr << "\b\b\b\b";
				std::cerr << std::setw(3) << p << "%";
			});
			msSimplifier.simplify();
			std::cerr << "\n";

			std::cerr << prefix << "Compacting MS complex...\n";
			msSimplified.compact();

			std::cerr << prefix << "Converting MS complex into network...     ";
			MsToNetworkGraphCreator networkGraphCreator(
						msSimplified, &networkGraph,
						[](int p) {
				std::cerr << "\b\b\b\b";
				std::cerr << std::setw(3) << p << "%";
			});
			networkGraphCreator.create();
			std::cerr << "\n";

		} else {
			std::cerr << prefix << "Computing representative network...     ";
			Network network;
			NetworkCreator networkCreator(
						striation, msComplex, &sandCache, &sortedPaths,
						parser.isSet(bidirectionalOption),
						delta, &network,
						[](int p) {
				std::cerr << "\b\b\b\b";
				std::cerr << std::setw(3) << p << "%";
			});
			networkCreator.create();
			std::cerr << "\n";

			std::cerr << prefix << "Converting network into graph...     ";
			NetworkGraphCreator networkGraphCreator(
						msComplex, network, &networkGraph,
						parser.isSet(simplifyOption),
						[](int p) {
				std::cerr << "\b\b\b\b";
				std::cerr << std::setw(3) << p << "%";
			});
			networkGraphCreator.create();
			std::cerr << "\n";
		}

		std::cerr << prefix << "Writing graph...\n";
		if (deltas.size() > 1) {
			if (parser.isSet(ipeOption)) {
				IpeWriter::writeIpeFile(
							heightMap, networkGraph,
							units, output + "-δ-" + deltaString + ".ipe");
			} else if (parser.isSet(linksOption)) {
				LinkSequence links(networkGraph);
				LinkSequenceWriter::writeLinkSequence(
							links,
							units,
							output + "-δ-" + deltaString + ".txt");
			} else {
				GraphWriter::writeGraph(
							networkGraph,
							units,
							output + "-δ-" + deltaString + ".txt");
			}
		} else {
			if (parser.isSet(ipeOption)) {
				IpeWriter::writeIpeFile(
							heightMap, networkGraph,
							units, output + ".ipe");
			} else if (parser.isSet(linksOption)) {
				LinkSequence links(networkGraph);
				LinkSequenceWriter::writeLinkSequence(
							links,
							units,
							output + ".txt");
			} else {
				GraphWriter::writeGraph(networkGraph,
										units,
										output + ".txt");
			}
		}
	}

	return 0;
}
