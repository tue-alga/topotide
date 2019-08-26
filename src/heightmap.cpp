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

bool HeightMap::isInBounds(int x, int y) const {
	return isInBounds(Coordinate(x, y));
}

bool HeightMap::isInBounds(Coordinate c) const {
	return c.m_x >= 0 && c.m_y >= 0 &&
		c.m_x < width() && c.m_y < height();
}

HeightMap::Boundary::Boundary() {}

HeightMap::Boundary::Boundary(Path source, Path top, Path sink, Path bottom) :
    source(source), top(top), sink(sink), bottom(bottom) {}

HeightMap::Boundary HeightMap::defaultBoundary() const {
	return Boundary(left(), top(), right(), bottom());
}

HeightMap::Path HeightMap::top() const {
	Path p;
	for (int x = 0; x < width(); x++) {
		p.m_points.emplace_back(x, 0);
	}
	return p;
}

HeightMap::Path HeightMap::bottom() const {
	Path p;
	for (int x = 0; x < width(); x++) {
		p.m_points.emplace_back(x, height() - 1);
	}
	return p;
}

HeightMap::Path HeightMap::left() const {
	Path p;
	for (int y = 0; y < height(); y++) {
		p.m_points.emplace_back(0, y);
	}
	return p;
}

HeightMap::Path HeightMap::right() const {
	Path p;
	for (int y = 0; y < height(); y++) {
		p.m_points.emplace_back(width() - 1, y);
	}
	return p;
}

QImage HeightMap::image() const {
	return _image;
}

void HeightMap::Path::removeSpikes() {
	for (int i = 1; i < m_points.size() - 1; i++) {
		// find tip of the spike
		if (m_points[i - 1] == m_points[i + 1]) {
			m_points.erase(m_points.begin() + i, m_points.begin() + i + 2);
			if (i > 1) {
				i -= 2;
			}
		}
	}
}
