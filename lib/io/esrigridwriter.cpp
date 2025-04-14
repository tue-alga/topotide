#include "esrigridwriter.h"

#include <QFile>
#include <QTextStream>

void EsriGridWriter::writeGridFile(const HeightMap& heightMap, const QString& fileName,
                                   const Units& units) {
	QFile file(fileName);
	file.open(QIODevice::WriteOnly | QIODevice::Text);
	QTextStream out(&file);

	out << "ncols " << heightMap.width() << "\n";
	out << "nrows " << heightMap.height() << "\n";
	out << "nodata_value " << -100000 << "\n";
	out << "cellsize " << units.m_xResolution << "\n\n";

	for (int y = 0; y < heightMap.height(); y++) {
		for (int x = 0; x < heightMap.width(); x++) {
			if (x > 0) {
				out << " ";
			}
			double elevation = heightMap.elevationAt(x, y);
			if (std::isnan(elevation)) {
				out << "-100000";
			} else {
				out << elevation;
			}
		}
		out << "\n";
	}
}
