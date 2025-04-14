#include "path.h"

#include <cassert>

Path::Path() :
    m_points() {}

Path::Path(HeightMap::Coordinate start, HeightMap::Coordinate end) :
    m_points({start, end}) {}

HeightMap::Coordinate& Path::start() {
	assert(length() >= 0);
	return m_points.front();
}

const HeightMap::Coordinate& Path::start() const {
	assert(length() >= 0);
	return m_points.front();
}

HeightMap::Coordinate& Path::end() {
	assert(length() >= 0);
	return m_points.back();
}

const HeightMap::Coordinate& Path::end() const {
	assert(length() >= 0);
	return m_points.back();
}

void Path::addPoint(HeightMap::Coordinate point) {
	m_points.push_back(point);
}

int Path::length() const {
	return m_points.size() - 1;
}

void Path::append(Path path) {
	assert(end() == path.start());
	for (int i = 0; i < path.length(); i++) {
		addPoint(path.m_points[i + 1]);
	}
}

void Path::appendRasterizedEdgeTo(HeightMap::Coordinate point) {
	// TODO this could be done more accurately with Bresenham's algorithm

	HeightMap::Coordinate p1 = end();
	HeightMap::Coordinate p2 = point;

	int distance = std::abs(p1.m_x - p2.m_x) + std::abs(p1.m_y - p2.m_y);

	switch (distance) {
		case 0:
			break;

		case 1:
			addPoint(p2);
			break;

		case 2:
			// special case for diagonal edges
			if (std::abs(p1.m_x - p2.m_x) == 1) {
				addPoint(HeightMap::Coordinate(p1.m_x, p2.m_y));
				addPoint(p2);
				break;
			} else {
				// fallthrough for horizontal / vertical edges
				[[fallthrough]];
			}

		default:
			// subdivide edge in two parts and recurse on those
			HeightMap::Coordinate pMid =
			        HeightMap::Coordinate::midpointBetween(p1, p2);
			appendRasterizedEdgeTo(pMid);
			appendRasterizedEdgeTo(p2);
	}
}

Path Path::rasterize() const {
	Path result;
	result.addPoint(start());
	for (int i = 0; i < length(); i++) {
		result.appendRasterizedEdgeTo(m_points[i + 1]);
	}
	result.removeSpikes();
	return result;
}

void Path::removeSpikes() {
	for (int i = 1; i < m_points.size() - 1; i++) {
		if (m_points[i - 1] == m_points[i]) {
			// found a double: erase it
			m_points.erase(m_points.begin() + i);
			i = std::max(0, i - 2);
			continue;
		}
		if (m_points[i - 1] == m_points[i + 1]) {
			// found a spike, m_points[i] is the tip
			m_points.erase(m_points.begin() + i, m_points.begin() + i + 2);
			i = std::max(0, i - 2);
		}
	}
}

std::optional<int> Path::closestTo(HeightMap::Coordinate c, double distanceLimit) const {
	return closestTo(Point{static_cast<double>(c.m_x), static_cast<double>(c.m_y), 0}, distanceLimit);
}

std::optional<int> Path::closestTo(Point p, double distanceLimit, std::function<bool(int)> accept) const {
	double minimumDistance = distanceLimit;
	std::optional<int> closest;
	for (int i = 0; i < m_points.size(); i++) {
		if (!accept(i)) {
			continue;
		}
		double distance = p.distanceTo(
		    Point{static_cast<double>(m_points[i].m_x), static_cast<double>(m_points[i].m_y), 0});
		if (distance < minimumDistance) {
			minimumDistance = distance;
			closest = i;
		}
	}
	return closest;
};
