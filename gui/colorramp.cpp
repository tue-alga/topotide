#include "colorramp.h"

ColorRamp::ColorRamp() : ColorRamp(QColor{0, 0, 0}, QColor{255, 255, 255}) {}

ColorRamp::ColorRamp(QColor start, QColor end) {
	m_stops.emplace_back(0.0, start);
	m_stops.emplace_back(1.0, end);
}

QColor ColorRamp::operator()(double value) const {
	assert(value >= 0.0 && value <= 1.0);
	if (value == 0.0) {
		return m_stops.front().second;
	} else if (value == 1.0) {
		return m_stops.back().second;
	} else {
		auto it =
		    std::lower_bound(m_stops.begin(), m_stops.end(), value,
		                     [](const ColorStop& stop, double value) { return stop.first < value; });
		ColorStop s1 = *(it - 1);
		ColorStop s2 = *it;
		double fraction = (value - s1.first) / (s2.first - s1.first);
		int r = (1 - fraction) * s1.second.red() + fraction * s2.second.red();
		int g = (1 - fraction) * s1.second.green() + fraction * s2.second.green();
		int b = (1 - fraction) * s1.second.blue() + fraction * s2.second.blue();
		return QColor{r, g, b};
	}
}

void ColorRamp::addStop(double value, QColor color) {
	assert(value >= 0.0 && value <= 1.0);
	if (value == 0.0) {
		m_stops.front().second = color;
	} else if (value == 1.0) {
		m_stops.back().second = color;
	} else {
		auto it =
			std::lower_bound(m_stops.begin(), m_stops.end(), value,
							[](const ColorStop& stop, double value) { return stop.first < value; });
		m_stops.insert(it, ColorStop{value, color});
	}
}

QImage ColorRamp::toImage() const {
	QImage result{256, 1, QImage::Format::Format_ARGB32};
	for (int x = 0; x < 256; x++) {
		result.setPixelColor(x, 0, (*this)(x / 256.0));
	}
	return result;
}
