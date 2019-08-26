#include "textfilereader.h"

#include <QFile>
#include <QStringList>
#include <QTextStream>

QImage
TextFileReader::readTextFile(
        const QString& fileName, QString& error, Units& units) {

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly)) {
		error = QString("File could not be read (%1)").arg(file.errorString());
		return QImage();
	}

	QTextStream stream(&file);
	QStringList numbers = stream.readAll().split(QRegExp("\\s+"),
	                                             QString::SkipEmptyParts);

	if (numbers.size() < 6) {
		error = QString("Premature end of file (should contain at least "
		                "six numbers indicating the width, height, "
		                "x-resolution, y-resolution, "
		                "minimum height, maximum height)");
		return QImage();
	}

	bool ok;

	// parse width and height
	int width = numbers[0].toInt(&ok);
	if (!ok) {
		error = QString("Width should be an integer (was [%1])").
		        arg(numbers[0]);
		return QImage();
	}
	if (width <= 0) {
		error = QString("Width should be positive (was [%1])").arg(width);
		return QImage();
	}

	int height = numbers[1].toInt(&ok);
	if (!ok) {
		error = QString("Height should be an integer (was [%1])").
		        arg(numbers[1]);
		return QImage();
	}
	if (height <= 0) {
		error = QString("Height should be positive (was [%1])").arg(height);
		return QImage();
	}

	double xRes = numbers[2].toDouble(&ok);
	if (!ok) {
		error = QString("x-resolution should be a number (was [%1])").
		        arg(numbers[2]);
		return QImage();
	}
	if (xRes <= 0) {
		error = QString("x-resolution should be positive (was [%1])").
		        arg(xRes);
		return QImage();
	}

	double yRes = numbers[3].toDouble(&ok);
	if (!ok) {
		error = QString("y-resolution should be a number (was [%1])").
		        arg(numbers[3]);
		return QImage();
	}
	if (yRes <= 0) {
		error = QString("y-resolution should be positive (was [%1])").
		        arg(yRes);
		return QImage();
	}

	double minHeight = numbers[4].toDouble(&ok);
	if (!ok) {
		error = QString("Minimum height should be a number (was [%1])").
		        arg(numbers[4]);
		return QImage();
	}

	double maxHeight = numbers[5].toDouble(&ok);
	if (!ok) {
		error = QString("Maximum height should be a number (was [%1])").
		        arg(numbers[5]);
		return QImage();
	}

	if (numbers.size() != 6 + width * height) {
		error = QString("File should contain %1 x %2 = %3 elevation measures "
		                "(encountered %4)")
		        .arg(width)
		        .arg(height)
		        .arg(width * height)
		        .arg(numbers.size() - 6);
		return QImage();
	}

	QImage image(width, height, QImage::Format_RGB888);
	for (int x = 0; x < width; ++x) {
		for (int y = 0; y < height; ++y) {
			double elevation = numbers[6 + width * y + x].toDouble(&ok);
			if (!ok) {
				error = QString("Elevation data should be numbers "
				                "(encountered [%1])").
				        arg(numbers[6 + width * y + x]);
				return QImage();
			}

			unsigned value = 0xff000000 + 0xffffff *
			        (elevation - minHeight) / (maxHeight - minHeight);
			image.setPixel(x, y, value);
		}
	}

	units.m_xResolution = xRes;
	units.m_yResolution = yRes;
	units.m_minElevation = minHeight;
	units.m_maxElevation = maxHeight;

	return image;
}
