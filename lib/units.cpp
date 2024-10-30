#include "units.h"

#include <cassert>
#include <cmath>

Units::Units() : m_xResolution(1), m_yResolution(1) {}

Units::Units(double xResolution, double yResolution)
    : m_xResolution(xResolution), m_yResolution(yResolution) {
	assert(m_xResolution > 0);
    assert(m_yResolution > 0);
}

double Units::length(Point p1, Point p2) const {
	return length(p1.x, p1.y, p2.x, p2.y);
}

double Units::length(double x1, double y1, double x2, double y2) const {
	double dx = m_xResolution * (x1 - x2);
	double dy = m_yResolution * (y1 - y2);
	return std::sqrt(dx * dx + dy * dy);
}

double Units::toRealVolume(double volume) const {
	return volume * m_xResolution * m_yResolution;
}

double Units::fromRealVolume(double volume) const {
	return volume / m_xResolution / m_yResolution;
}
