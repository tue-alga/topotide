#include "boundaryreader.h"

#include <QFile>
#include <QStringList>
#include <QTextStream>

#include "../inputdcel.h"

HeightMap::Boundary
BoundaryReader::readBoundary(const QString& fileName,
                             const HeightMap& map, QString& error) {

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
		error = QString("File could not be read (%1)").arg(file.errorString());
		return {};
	}

	QTextStream stream(&file);
	QStringList numbers = stream.readAll().split(QRegExp("\\s+"),
	                                             QString::SkipEmptyParts);

	if (numbers.size() < 4) {
		error = QString("Premature end of file (should contain at least "
		                "four numbers)");
		return {};
	}

	bool ok;

	// parse lengths
	int sourceLength = numbers[0].toInt(&ok);
	if (!ok) {
		error = QString("Source length should be an integer (was [%1])").
		        arg(numbers[0]);
		return {};
	}
	if (sourceLength <= 0) {
		error = QString("Source length should be positive (was [%1])")
		        .arg(sourceLength);
		return {};
	}

	int sinkLength = numbers[1].toInt(&ok);
	if (!ok) {
		error = QString("Sink length should be an integer (was [%1])").
		        arg(numbers[1]);
		return {};
	}
	if (sinkLength <= 0) {
		error = QString("Sink length should be positive (was [%1])")
		        .arg(sinkLength);
		return {};
	}

	int topLength = numbers[2].toInt(&ok);
	if (!ok) {
		error = QString("Top length should be an integer (was [%1])").
		        arg(numbers[2]);
		return {};
	}
	if (topLength <= 0) {
		error = QString("Top length should be positive (was [%1])")
		        .arg(topLength);
		return {};
	}

	int bottomLength = numbers[3].toInt(&ok);
	if (!ok) {
		error = QString("Bottom length should be an integer (was [%1])").
		        arg(numbers[3]);
		return {};
	}
	if (bottomLength <= 0) {
		error = QString("Bottom length should be positive (was [%1])")
		        .arg(bottomLength);
		return {};
	}

	int expectedCoordCount =
	        sourceLength + sinkLength + topLength + bottomLength;
	if (numbers.size() !=
	        4 + 2 * (sourceLength + sinkLength + topLength + bottomLength)) {
		error = QString("File should contain %1 x- and y-coordinates "
		                "(encountered %2)")
		        .arg(2 * expectedCoordCount)
		        .arg(numbers.size() - 4);
		return {};
	}

	int index = 4;

	HeightMap::Boundary boundary;
	try {
		readPath(boundary.source, numbers, sourceLength, map, index);
		readPath(boundary.sink, numbers, sinkLength, map, index);
		readPath(boundary.top, numbers, topLength, map, index);
		readPath(boundary.bottom, numbers, bottomLength, map, index);
	} catch (std::runtime_error& e) {
		error = e.what();
		return {};
	}

	// consistency checks
	HeightMap::Coordinate topStart = boundary.top.m_points[0];
	HeightMap::Coordinate topEnd = boundary.top.m_points[boundary.top.m_points.size() - 1];
	HeightMap::Coordinate bottomStart = boundary.bottom.m_points[0];
	HeightMap::Coordinate bottomEnd = boundary.bottom.m_points[boundary.bottom.m_points.size() - 1];
	HeightMap::Coordinate sourceStart = boundary.source.m_points[0];
	HeightMap::Coordinate sourceEnd = boundary.source.m_points[boundary.source.m_points.size() - 1];
	HeightMap::Coordinate sinkStart = boundary.sink.m_points[0];
	HeightMap::Coordinate sinkEnd = boundary.sink.m_points[boundary.sink.m_points.size() - 1];

	if (topStart != sourceStart) {
		error = "The start of the top is not equal to the start of the source";
		return {};
	}
	if (topEnd != sinkStart) {
		error = "The end of the top is not equal to the start of the sink";
		return {};
	}
	if (bottomStart != sourceEnd) {
		error = "The start of the bottom is not equal to the end of the source";
		return {};
	}
	if (bottomEnd != sinkEnd) {
		error = "The end of the bottom is not equal to the end of the sink";
		return {};
	}

	// check if all non-endpoints are visited only once
	std::unordered_set<HeightMap::Coordinate, CoordinateHasher> visited;
	try {
		checkNoDouble(boundary.source, visited);
	} catch (std::runtime_error& e) {
		error = e.what();
		return {};
	}

	return boundary;
}

void
BoundaryReader::readPath(HeightMap::Path& path, const QStringList& numbers,
                         int length, const HeightMap& map, int& index) {
	bool ok;
	int px = -1;
	int py = -1;
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
		// ignore duplicated points
		if (px == x && py == y) {
			continue;
		}
		if (!map.isInBounds(x, y)) {
			std::string error = "Coordinate [" +
			                    numbers[index - 2].toStdString() +
			                    ", " +
			                    numbers[index - 1].toStdString() +
			        "] is out of bounds";
			throw std::runtime_error(error);
		}
		if (i != 0) {
			if (px != x && py != y) {
				std::string error = "Illegal diagonal edge "
				                    "(" + std::to_string(px) + ", " +
				                    std::to_string(py) + ") -> "
				                    "(" + std::to_string(x) + ", " +
				                    std::to_string(y) + ")";
				throw std::runtime_error(error);
			}
			if (std::abs(px - x) > 1 || std::abs(py - y) > 1) {
				std::string error = "Illegal long edge "
				                    "(" + std::to_string(px) + ", " +
				                    std::to_string(py) + ") -> "
				                    "(" + std::to_string(x) + ", " +
				                    std::to_string(y) + ")";
				throw std::runtime_error(error);
			}
		}
		path.m_points.emplace_back(x, y);
		px = x;
		py = y;
	}

	path.removeSpikes();
}

void
BoundaryReader::checkNoDouble(
        const HeightMap::Path& path,
        std::unordered_set<HeightMap::Coordinate, CoordinateHasher>& visited) {
	for (int i = 1; i < path.m_points.size() - 1; i++) {
		HeightMap::Coordinate v = path.m_points[i];
		if (visited.find(v) != visited.end()) {
			throw std::runtime_error(
			        "Point visited twice: (" +
			            std::to_string(v.m_x) + ", " +
			            std::to_string(v.m_y) + ")");
		}
		visited.insert(v);
	}
}
