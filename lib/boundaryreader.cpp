#include "boundaryreader.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

Boundary
BoundaryReader::readBoundary(const QString& fileName,
                             int width, int height, QString& error) {

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
		error = QString("File could not be read (%1)").arg(file.errorString());
		return Boundary(width, height);
	}

	QTextStream stream(&file);
	QStringList numbers = stream.readAll().split(QRegularExpression("\\s+"),
	                                             Qt::SkipEmptyParts);

	if (numbers.size() < 4) {
		error = QString("Premature end of file (should contain at least "
		                "four numbers)");
		return Boundary(width, height);
	}

	bool ok;

	// parse lengths
	int sourceLength = numbers[0].toInt(&ok);
	if (!ok) {
		error = QString("Source length should be an integer (was [%1])").
		        arg(numbers[0]);
		return Boundary(width, height);
	}
	if (sourceLength <= 0) {
		error = QString("Source length should be positive (was [%1])")
		        .arg(sourceLength);
		return Boundary(width, height);
	}

	int topLength = numbers[1].toInt(&ok);
	if (!ok) {
		error = QString("Top length should be an integer (was [%1])").
		        arg(numbers[1]);
		return Boundary(width, height);
	}
	if (topLength <= 0) {
		error = QString("Top length should be positive (was [%1])")
		        .arg(topLength);
		return Boundary(width, height);
	}

	int sinkLength = numbers[2].toInt(&ok);
	if (!ok) {
		error = QString("Sink length should be an integer (was [%1])").
		        arg(numbers[2]);
		return Boundary(width, height);
	}
	if (sinkLength <= 0) {
		error = QString("Sink length should be positive (was [%1])")
		        .arg(sinkLength);
		return Boundary(width, height);
	}

	int bottomLength = numbers[3].toInt(&ok);
	if (!ok) {
		error = QString("Bottom length should be an integer (was [%1])").
		        arg(numbers[3]);
		return Boundary(width, height);
	}
	if (bottomLength <= 0) {
		error = QString("Bottom length should be positive (was [%1])")
		        .arg(bottomLength);
		return Boundary(width, height);
	}

	int expectedCoordCount =
	        sourceLength + topLength + sinkLength + bottomLength;
	if (numbers.size() !=
	        4 + 2 * (sourceLength + topLength + sinkLength + bottomLength)) {
		error = QString("File should contain %1 x- and y-coordinates "
		                "(encountered %2)")
		        .arg(2 * expectedCoordCount)
		        .arg(numbers.size() - 4);
		return Boundary(width, height);
	}

	int index = 4;

	Path source, top, sink, bottom;
	try {
		readPath(source, numbers, sourceLength, width, height, index);
		readPath(top, numbers, topLength, width, height, index);
		readPath(sink, numbers, sinkLength, width, height, index);
		readPath(bottom, numbers, bottomLength, width, height, index);
	} catch (std::runtime_error& e) {
		error = e.what();
		return Boundary(width, height);
	}

	// consistency checks
	if (source.end() != top.start()) {
		error = "The source does not connect to the top";
		return Boundary(width, height);
	}
	if (top.end() != sink.start()) {
		error = "The top does not connect to the sink";
		return Boundary(width, height);
	}
	if (sink.end() != bottom.start()) {
		error = "The sink does not connect to the bottom";
		return Boundary(width, height);
	}
	if (bottom.end() != source.start()) {
		error = "The bottom does not connect to the source";
		return Boundary(width, height);
	}

	source.append(top);
	source.append(sink);
	source.append(bottom);
	Boundary boundary(source);

	boundary.addPermeableRegion({0, sourceLength - 1});
	boundary.addPermeableRegion(
	    {sourceLength + topLength - 2, sourceLength + topLength + sinkLength - 3});

	return boundary;
}

void
BoundaryReader::readPath(Path& path, const QStringList& numbers,
                         int length, int width, int height, int& index) {
	bool ok;
	for (int i = 0; i < length; i++) {
		int x = numbers[index].toInt(&ok);
		if (!ok) {
			std::string error = "Coordinate [" + numbers[index].toStdString() +
			        "] should be an integer";
			throw std::runtime_error(error);
		}
		index++;
		int y = numbers[index].toInt(&ok);
		if (!ok) {
			std::string error = "Coordinate [" + numbers[index].toStdString() +
			        "] should be an integer";
			throw std::runtime_error(error);
		}
		index++;
		if (x < 0 || x >= width || y < 0 || y >= height) {
			std::string error = "Coordinate [" + numbers[index - 2].toStdString() + ", " +
			                    numbers[index - 1].toStdString() + "] is out of bounds";
			throw std::runtime_error(error);
		}
		path.m_points.emplace_back(x, y);
	}
}
