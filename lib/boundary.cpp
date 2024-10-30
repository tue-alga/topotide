#include "boundary.h"
#include <unordered_set>

Boundary::Boundary() :
    m_source(),
    m_top(),
    m_sink(),
    m_bottom() {}

Boundary::Boundary(int width, int height)
    : m_source(HeightMap::Coordinate{0, height - 1}, HeightMap::Coordinate{0, 0}),
      m_top(HeightMap::Coordinate{0, 0}, HeightMap::Coordinate{width - 1, 0}),
      m_sink(HeightMap::Coordinate{width - 1, 0}, HeightMap::Coordinate{width - 1, height - 1}),
      m_bottom(HeightMap::Coordinate{width - 1, height - 1}, HeightMap::Coordinate{0, height - 1}) {}

Boundary::Boundary(const HeightMap& map) : Boundary(map.width(), map.height()) {}

Boundary::Boundary(Path source, Path top, Path sink, Path bottom) :
    m_source(source),
    m_top(top),
    m_sink(sink),
    m_bottom(bottom) {}

Boundary Boundary::rasterize() const {
	Path source = m_source.rasterize();
	Path top = m_top.rasterize();
	Path sink = m_sink.rasterize();
	Path bottom = m_bottom.rasterize();

	auto removeCommonParts = [](Path& p1, Path& p2) {
		int toRemove = 0;
		for (int i = 1; i < std::min(p1.length(), p2.length()); i++) {
			HeightMap::Coordinate c1 = p1.m_points[p1.length() - i];
			HeightMap::Coordinate c2 = p2.m_points[i];
			if (c1 == c2) {
				toRemove = i;
			}
			if (abs(c1.m_x - c2.m_x) > 2 ||
			        abs(c1.m_y - c2.m_y) > 2) {
				break;
			}
		}
		p1.m_points.erase(p1.m_points.end() - toRemove, p1.m_points.end());
		p2.m_points.erase(p2.m_points.begin(), p2.m_points.begin() + toRemove);
	};
	removeCommonParts(source, top);
	removeCommonParts(top, sink);
	removeCommonParts(sink, bottom);
	removeCommonParts(bottom, source);

	Boundary result(source, top, sink, bottom);

	return result;
}

void Boundary::ensureConnection() {
	m_source.end() = m_top.start();
	m_top.end() = m_sink.start();
	m_sink.end() = m_bottom.start();
	m_bottom.end() = m_source.start();
}

bool Boundary::isValid() const {
	auto CoordinateHasher = [](const HeightMap::Coordinate& c) {
		return std::hash<int>()(c.m_x) ^ (std::hash<int>()(c.m_y) << 1);
	};
	std::unordered_set<HeightMap::Coordinate, decltype(CoordinateHasher)> visited(0, CoordinateHasher);
	for (const Path& p : {m_source, m_top, m_sink, m_bottom}) {
		for (int i = 0; i < p.m_points.size() - 1; i++) {
			if (visited.find(p.m_points[i]) != visited.end()) {
				return false;
			}
			visited.insert(p.m_points[i]);
		}
	}
	return true;
}

bool Boundary::isClockwise() const {
	long area = 0;
	auto addSignedAreaContribution = [&area](std::vector<HeightMap::Coordinate> coords) {
		for (int i = 0; i < coords.size() - 1; i++) {
			area += (coords[i].m_x * coords[i + 1].m_y - coords[i + 1].m_x * coords[i].m_y);
		}
	};
	addSignedAreaContribution(m_source.m_points);
	addSignedAreaContribution(m_top.m_points);
	addSignedAreaContribution(m_sink.m_points);
	addSignedAreaContribution(m_bottom.m_points);
	return area > 0;
}
