#include "boundary.h"

#include <cassert>
#include <optional>
#include <unordered_set>

Boundary::Boundary() : m_path() {}

Boundary::Boundary(int width, int height) {
	m_path.addPoint(HeightMap::Coordinate{0, height - 1});
	m_path.addPoint(HeightMap::Coordinate{0, 0});
	m_path.addPoint(HeightMap::Coordinate{width - 1, 0});
	m_path.addPoint(HeightMap::Coordinate{width - 1, height - 1});
	m_path.addPoint(HeightMap::Coordinate{0, height - 1});

	addPermeableRegion({0, 1});
	addPermeableRegion({2, 3});
}

Boundary::Boundary(const HeightMap& map) : Boundary(map.width(), map.height()) {}

Boundary::Boundary(Path path) : m_path(path) {}

const Path& Boundary::path() const {
	return m_path;
}

void Boundary::movePoint(int index, HeightMap::Coordinate c) {
	assert(index < m_path.m_points.size());
	m_path.m_points[index] = c;
	if (index == 0) {
		m_path.m_points.back() = c;
	} else if (index == m_path.m_points.size() - 1) {
		m_path.m_points.front() = c;
	}
}

void Boundary::insertPoint(int index, HeightMap::Coordinate c) {
	assert(index <= m_path.m_points.size());
	m_path.m_points.insert(m_path.m_points.begin() + index, c);
	movePoint(index, c);
	for (Region& region : m_permeableRegions) {
		if (region.m_start >= index) {
			region.m_start++;
		}
		if (region.m_end >= index) {
			region.m_end++;
		}
	}
}

void Boundary::addPermeableRegion(Region region) {
	m_permeableRegions.push_back(region);
}

void Boundary::setLastPermeableRegion(Region region) {
	m_permeableRegions.back() = region;
}

Boundary::Region Boundary::lastPermeableRegion() const {
	return m_permeableRegions.back();
}

const std::vector<Boundary::Region>& Boundary::permeableRegions() const {
	return m_permeableRegions;
}

void Boundary::removePermeableRegions() {
	m_permeableRegions.clear();
}

std::vector<Boundary::Region> Boundary::impermeableRegions() const {
	std::vector<Boundary::Region> result;
	for (int i = 0; i < m_permeableRegions.size(); i++) {
		result.emplace_back(m_permeableRegions[i].m_end,
		                    m_permeableRegions[(i + 1) % m_permeableRegions.size()].m_start);
	}
	return result;
}

Boundary Boundary::rasterize() const {
	Path path = m_path.rasterize();
	path.removeSpikes();
	assert(path.length() > 0);

	Boundary result(path);

	for (const Region& region : m_permeableRegions) {
		HeightMap::Coordinate start = m_path.m_points[region.m_start];
		HeightMap::Coordinate end = m_path.m_points[region.m_end];
		result.addPermeableRegion(Region{*path.closestTo(start), *path.closestTo(end)});
	}

	return result;
}

bool Boundary::isValid() const {
	auto CoordinateHasher = [](const HeightMap::Coordinate& c) {
		return std::hash<int>()(c.m_x) ^ (std::hash<int>()(c.m_y) << 1);
	};
	std::unordered_set<HeightMap::Coordinate, decltype(CoordinateHasher)> visited(0, CoordinateHasher);
	for (int i = 0; i < m_path.m_points.size() - 1; i++) {
		if (visited.find(m_path.m_points[i]) != visited.end()) {
			return false;
		}
		visited.insert(m_path.m_points[i]);
	}
	return true;
}

bool Boundary::isClockwise(const Path& path) {
	long area = 0;
	for (int i = 0; i < path.m_points.size() - 1; i++) {
		area += (path.m_points[i].m_x * path.m_points[i + 1].m_y -
		         path.m_points[i + 1].m_x * path.m_points[i].m_y);
	}
	return area > 0;
}
