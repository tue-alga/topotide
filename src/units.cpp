#include "units.h"

#include <cassert>
#include <cmath>

Units::Units() :
    m_xResolution(1),
    m_yResolution(1),
    m_minElevation(0),
    m_maxElevation(10) {
}

Units::Units(double xResolution,
             double yResolution,
             double minElevation,
             double maxElevation) :
    m_xResolution(xResolution),
    m_yResolution(yResolution),
    m_minElevation(minElevation),
    m_maxElevation(maxElevation) {

    assert(m_xResolution > 0);
    assert(m_yResolution > 0);
    assert(m_minElevation < m_maxElevation);
}

double Units::length(Point p1, Point p2) const {
	return length(p1.x, p1.y, p2.x, p2.y);
}

double Units::length(int x1, int y1, int x2, int y2) const {
	int dx = m_xResolution * (x1 - x2);
	int dy = m_yResolution * (y1 - y2);
	return std::sqrt(dx * dx + dy * dy);
}

double Units::toRealElevation(int elevation) const {
	double fraction = (double) elevation / 0xffffff;
	return fraction * m_maxElevation + (1 - fraction) * m_minElevation;
}

double Units::toRealVolume(double volume) const {
	double scaledVolume = (double) volume / 0xffffff;
	return scaledVolume * m_xResolution * m_yResolution *
	        (m_maxElevation - m_minElevation);
}

double Units::fromRealVolume(double volume) const {
	double scaledVolume = volume / m_xResolution / m_yResolution /
	         (m_maxElevation - m_minElevation);
	return scaledVolume * 0xffffff;
}
