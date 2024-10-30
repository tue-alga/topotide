#include "heightmap.h"
#include <limits>

HeightMap::HeightMap() : HeightMap(0, 0) {}

HeightMap::HeightMap(int width, int height) : m_width(width), m_height(height) {
	m_data.resize(width * height, nodata);
}

double HeightMap::elevationAt(int x, int y) const {
	return m_data[m_width * y + x];
}

double HeightMap::elevationAt(Coordinate c) const {
	return elevationAt(c.m_x, c.m_y);
}

void HeightMap::setElevationAt(int x, int y, double elevation) {
	m_data[m_width * y + x] = elevation;
}

void HeightMap::setElevationAt(Coordinate c, double elevation) {
	setElevationAt(c.m_x, c.m_y, elevation);
}

int HeightMap::width() const {
	return m_width;
}

int HeightMap::height() const {
	return m_height;
}

bool HeightMap::isEmpty() const {
	return m_width == 0 && m_height == 0;
}

HeightMap::Coordinate HeightMap::Coordinate::midpointBetween(HeightMap::Coordinate c1,
                                                             HeightMap::Coordinate c2) {
	return Coordinate((c1.m_x + c2.m_x) / 2,
	                  (c1.m_y + c2.m_y) / 2);
}

bool HeightMap::isInBounds(int x, int y) const {
	return isInBounds(Coordinate(x, y));
}

bool HeightMap::isInBounds(Coordinate c) const {
	return c.m_x >= 0 && c.m_y >= 0 &&
		c.m_x < width() && c.m_y < height();
}

double HeightMap::minimumElevation() const {
	double minimum = std::numeric_limits<double>::infinity();
	for (int i = 0; i < m_data.size(); i++) {
		if (m_data[i] != nodata) {
			minimum = std::min(minimum, m_data[i]);
		}
	}
	return minimum;
}

double HeightMap::maximumElevation() const {
	double maximum = -std::numeric_limits<double>::infinity();
	for (int i = 0; i < m_data.size(); i++) {
		if (m_data[i] != nodata) {
			maximum = std::max(maximum, m_data[i]);
		}
	}
	return maximum;
}

HeightMap::Coordinate HeightMap::topLeft() const {
	return Coordinate(0, 0);
}

HeightMap::Coordinate HeightMap::topRight() const {
	return Coordinate(width() - 1, 0);
}

HeightMap::Coordinate HeightMap::bottomLeft() const {
	return Coordinate(0, height() - 1);
}

HeightMap::Coordinate HeightMap::bottomRight() const {
	return Coordinate(width() - 1, height() - 1);
}