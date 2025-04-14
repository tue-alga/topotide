#ifndef COLORRAMP_H
#define COLORRAMP_H

#include <QColor>
#include <QImage>

/// A color ramp used to map elevations (between 0.0 and 1.0) to color values.
class ColorRamp {
	public:
		ColorRamp();
		ColorRamp(QColor start, QColor end);
		void addStop(double value, QColor color);
		void removeStop(double value);
		QColor operator()(double value) const;

		/// Outputs the color ramp as an 256x1 image that can be passed to a
		/// fragment shader to sample elevation colors from.
		QImage toImage() const;

	private:
		using ColorStop = std::pair<double, QColor>;
		/// The list of stops, sorted by value. Always starts and ends with
		/// stops for the values 0.0 and 1.0.
		std::vector<ColorStop> m_stops;
};

#endif // COLORRAMP_H
