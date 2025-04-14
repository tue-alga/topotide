#include "gdalreader.h"

#include <cpl_error.h>
#include <gdal_priv.h>

HeightMap
GdalReader::readGdalFile(
        const QString& fileName, QString& error, Units& units) {

	std::string fileNameString = fileName.toStdString();
	const char* fileNameCharArray = fileNameString.c_str();

	CPLSetErrorHandler([](CPLErr, CPLErrorNum, const char*) {
		// suppress stderr output
	});

	GDALAllRegister();
	GDALDatasetUniquePtr dataset(GDALDataset::FromHandle(GDALOpen(fileNameCharArray, GA_ReadOnly)));
	if (!dataset) {
		error = CPLGetLastErrorMsg();
		return HeightMap();
	}
	if (dataset->GetRasterCount() < 1) {
		error = "Dataset did not have any bands";
		return HeightMap();
	}
	GDALRasterBand* band = dataset->GetRasterBand(1);
	int width = band->GetXSize();
	int height = band->GetYSize();

	int hasNoData;
	double nodata = band->GetNoDataValue(&hasNoData);

	std::vector<double> buffer(width * height);
	CPLErr ioError = band->RasterIO(GF_Read, 0, 0, width, height, buffer.data(), width, height, GDT_Float64, 0, 0);
	if (ioError != 0) {
		error = CPLGetLastErrorMsg();
		return HeightMap();
	}

	double minHeight = std::numeric_limits<double>::infinity();
	double maxHeight = -std::numeric_limits<double>::infinity();
	for (int i = 0; i < buffer.size(); i++) {
		if (!hasNoData || buffer[i] != nodata) {
			minHeight = std::min(minHeight, buffer[i]);
			maxHeight = std::max(maxHeight, buffer[i]);
		}
	}

	HeightMap heightMap(width, height);
	int i = 0;
	for (int y = 0; y < height; ++y) {
		for (int x = 0; x < width; ++x) {
			double elevation = buffer[i];
			if (!hasNoData || elevation != nodata) {
				heightMap.setElevationAt(x, y, elevation);
			}
			i++;
		}
	}
	return heightMap;
}
