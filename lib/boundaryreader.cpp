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

	Boundary boundary;
	try {
		readPath(boundary.m_source, numbers, sourceLength, width, height, index);
		readPath(boundary.m_top, numbers, topLength, width, height, index);
		readPath(boundary.m_sink, numbers, sinkLength, width, height, index);
		readPath(boundary.m_bottom, numbers, bottomLength, width, height, index);
	} catch (std::runtime_error& e) {
		error = e.what();
		return Boundary(width, height);
	}

	// consistency checks
	if (boundary.m_source.end() != boundary.m_top.start()) {
		error = "The source does not connect to the top";
		return Boundary(width, height);
	}
	if (boundary.m_top.end() != boundary.m_sink.start()) {
		error = "The top does not connect to the sink";
		return Boundary(width, height);
	}
	if (boundary.m_sink.end() != boundary.m_bottom.start()) {
		error = "The sink does not connect to the bottom";
		return Boundary(width, height);
	}
	if (boundary.m_bottom.end() != boundary.m_source.start()) {
		error = "The bottom does not connect to the source";
		return Boundary(width, height);
	}

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
