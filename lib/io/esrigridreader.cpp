#include "esrigridreader.h"

#include <QFile>
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>

#include <limits>
#include <stdexcept>

HeightMap
EsriGridReader::readGridFile(
        const QString& fileName, QString& error, Units& units) {
	HeightMap heightMap = readGridFile(fileName, error, units, QLocale::C);
	if (!heightMap.isEmpty()) {
		return heightMap;
	}

	// some ESRI grid files in practice use a comma as a decimal separator,
	// so if we have an error, retry with the Dutch locale
	// we ignore the error here, because if both parsing attempts failed, then
	// it is most likely that something else was wrong, so then we want the
	// original error message to explain that
	QString _;
	heightMap = readGridFile(fileName, _, units, QLocale::Dutch);
	if (!heightMap.isEmpty()) {
		error = "";
		return heightMap;
	}
	return HeightMap();
}

HeightMap
EsriGridReader::readGridFile(
        const QString& fileName, QString& error, Units& units, QLocale locale) {

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
		error = QString("File could not be read (%1)").arg(file.errorString());
		return HeightMap();
	}

	QTextStream stream(&file);
	QStringList tokens = stream.readAll().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);

	// first build a map of key-value pairs in the header
	Header header;
	bool ok;
	int i = 0;
	while (tokens.size() > i && tokens[i][0].isLetter()) {
		QString key = tokens[i];
		if (tokens.size() < i + 2) {
			error = QString("Missing value for %1").arg(key);
			return HeightMap();
		}
		int intValue = tokens[i + 1].toInt(&ok);
		if (ok) {
			header[key.toLower()] = intValue;
		} else {
			double doubleValue = locale.toDouble(tokens[i + 1], &ok);
			if (ok) {
				header[key.toLower()] = doubleValue;
			} else {
				error = QString("%1 should be numeric (was [%2])").
						arg(key, tokens[i + 1]);
				return HeightMap();
			}
		}
		i += 2;
	}

	int width, height;
	double res, nodata;
	try {
		width = getPositiveIntFromHeader(header, "ncols");
		height = getPositiveIntFromHeader(header, "nrows");
		nodata = getNumberFromHeader(header, "nodata_value");
		res = getNumberFromHeader(header, "cellsize");
	} catch (std::runtime_error& e) {
		error = e.what();
		return HeightMap();
	}

	if (tokens.size() - i != width * height) {
		error = QString("File should contain %1 x %2 = %3 elevation measures "
		                "(encountered %4)")
		            .arg(width).arg(height).arg(width * height)
		            .arg(tokens.size() - i);
		return HeightMap();
	}

	double minHeight = std::numeric_limits<double>::infinity();
	double maxHeight = -std::numeric_limits<double>::infinity();
	std::vector<double> elevations;
	elevations.reserve(width * height);
	for (; i < tokens.size(); i++) {
		double elevation = locale.toDouble(tokens[i], &ok);
		if (!ok) {
			error = QString("Elevation data should be numbers "
							"(encountered [%1])")
						.arg(tokens[i]);
			return HeightMap();
		}
		elevations.push_back(elevation);
		if (elevation != nodata) {
			minHeight = std::min(minHeight, elevation);
			maxHeight = std::max(maxHeight, elevation);
		}
	}
	
	HeightMap heightMap(width, height);
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			double elevation = elevations[width * y + x];
			if (elevation != nodata) {
				heightMap.setElevationAt(x, y, elevation);
			}
			i++;
		}
	}
	units.m_xResolution = res;
	units.m_yResolution = res;
	return heightMap;
}

int EsriGridReader::getIntFromHeader(Header& header, QString key) {
	if (header.find(key) == header.end()) {
		throw std::runtime_error(QString("Missing value for %1").arg(key).toStdString());
	}
	if (!std::holds_alternative<int>(header[key])) {
		throw std::runtime_error(QString("%1 should be an integer (was [%2])")
		                             .arg(key).arg(std::get<double>(header["ncols"]))
		                             .toStdString());
	}
	return std::get<int>(header[key]);
}

int EsriGridReader::getPositiveIntFromHeader(Header& header, QString key) {
	int result = getIntFromHeader(header, key);
	if (result <= 0) {
		throw std::runtime_error(
			QString("%1 should be positive (was [%2])").arg(key).arg(result).toStdString());
	}
	return result;
}

double EsriGridReader::getNumberFromHeader(Header& header, QString key) {
	double result;
	if (header.find(key) == header.end()) {
		throw std::runtime_error(QString("Missing value for %1").arg(key).toStdString());
	}
	if (std::holds_alternative<double>(header[key])) {
		result = std::get<double>(header[key]);
	} else if (std::holds_alternative<int>(header[key])) {
		result = std::get<int>(header[key]);
	} else {
		throw std::runtime_error(QString("%1 should be a number (was [%2])")
		                             .arg(key).arg(std::get<double>(header["ncols"]))
		                             .toStdString());
	}
	return result;
}
