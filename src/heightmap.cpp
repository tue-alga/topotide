#include "heightmap.h"

HeightMap::HeightMap() = default;

HeightMap::HeightMap(QImage image) : _image(image) {
}

int HeightMap::elevationAt(int x, int y) const {
	return (qRed(_image.pixel(x, y)) << 16) |
	        (qGreen(_image.pixel(x, y)) << 8) |
	        (qBlue(_image.pixel(x, y)));
}

int HeightMap::width() const {
	return _image.width();
}

int HeightMap::height() const {
	return _image.height();
}

HeightMap::Coordinate HeightMap::Coordinate::midpointBetween(
        HeightMap::Coordinate c1, HeightMap::Coordinate c2) {
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

QImage HeightMap::image() const {
	return _image;
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
