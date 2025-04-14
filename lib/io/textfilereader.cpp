#include "textfilereader.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

HeightMap
TextFileReader::readTextFile(
        const QString& fileName, QString& error, Units& units) {

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
		error = QString("File could not be read (%1)").arg(file.errorString());
		return HeightMap();
	}

	QTextStream stream(&file);
	QStringList numbers = stream.readAll().split(QRegularExpression("\\s+"),
	                                             Qt::SkipEmptyParts);

	if (numbers.size() < 6) {
		error = QString("Premature end of file (should contain at least "
		                "six numbers indicating the width, height, "
		                "x-resolution, y-resolution, "
		                "minimum height, maximum height)");
		return HeightMap();
	}

	bool ok;

	// parse width and height
	int width = numbers[0].toInt(&ok);
	if (!ok) {
		error = QString("Width should be an integer (was [%1])").
		        arg(numbers[0]);
		return HeightMap();
	}
	if (width <= 0) {
		error = QString("Width should be positive (was [%1])").arg(width);
		return HeightMap();
	}

	int height = numbers[1].toInt(&ok);
	if (!ok) {
		error = QString("Height should be an integer (was [%1])").
		        arg(numbers[1]);
		return HeightMap();
	}
	if (height <= 0) {
		error = QString("Height should be positive (was [%1])").arg(height);
		return HeightMap();
	}

	double xRes = numbers[2].toDouble(&ok);
	if (!ok) {
		error = QString("x-resolution should be a number (was [%1])").
		        arg(numbers[2]);
		return HeightMap();
	}
	if (xRes <= 0) {
		error = QString("x-resolution should be positive (was [%1])").
		        arg(xRes);
		return HeightMap();
	}

	double yRes = numbers[3].toDouble(&ok);
	if (!ok) {
		error = QString("y-resolution should be a number (was [%1])").
		        arg(numbers[3]);
		return HeightMap();
	}
	if (yRes <= 0) {
		error = QString("y-resolution should be positive (was [%1])").
		        arg(yRes);
		return HeightMap();
	}

	// minHeight and maxHeight are not used anymore, but are still read for
	// compatibility with old files
	[[maybe_unused]] double minHeight = numbers[4].toDouble(&ok);
	if (!ok) {
		error = QString("Minimum height should be a number (was [%1])").
		        arg(numbers[4]);
		return HeightMap();
	}

	[[maybe_unused]] double maxHeight = numbers[5].toDouble(&ok);
	if (!ok) {
		error = QString("Maximum height should be a number (was [%1])").
		        arg(numbers[5]);
		return HeightMap();
	}

	if (numbers.size() != 6 + width * height) {
		error = QString("File should contain %1 x %2 = %3 elevation measures "
		                "(encountered %4)")
		        .arg(width)
		        .arg(height)
		        .arg(width * height)
		        .arg(numbers.size() - 6);
		return HeightMap();
	}

	HeightMap heightMap(width, height);
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			double elevation = numbers[6 + width * y + x].toDouble(&ok);
			if (!ok) {
				error = QString("Elevation data should be numbers "
				                "(encountered [%1])").
				        arg(numbers[6 + width * y + x]);
				return HeightMap();
			}

			heightMap.setElevationAt(x, y, elevation);
		}
	}
	units.m_xResolution = xRes;
	units.m_yResolution = yRes;
	return heightMap;
}
