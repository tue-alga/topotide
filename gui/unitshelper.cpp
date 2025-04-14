#include "unitshelper.h"

#include <cmath>

#include "heightmap.h"

QString UnitsHelper::formatElevation(double meters) {
	if (std::isnan(meters)) {
		return "(no data)";
	}
	int precision;
	if (std::abs(meters) >= 100) {
		precision = 0;
	} else {
		precision = 1;
	}
	return QString("%1 m").arg(meters, 0, 'f', precision, '0');
}

QString UnitsHelper::formatVolume(double cubicMeters) {
	int precision;

	if (cubicMeters >= 1000) {
		// display in m³ in scientific notation
		return QString("%1 m³").arg(toScientificNotation(cubicMeters));
	}

	if (cubicMeters >= 1) {
		// display in m³
		if (cubicMeters >= 100) {
			precision = 0;
		} else if (cubicMeters >= 10) {
			precision = 1;
		} else {
			precision = 2;
		}
		return QString("%1 m³").arg(cubicMeters, 0, 'f', precision, '0');
	}

	double cubicDecimeters = cubicMeters * 1000;

	if (cubicDecimeters >= 1) {
		// display in dm³
		if (cubicDecimeters >= 100) {
			precision = 0;
		} else if (cubicDecimeters >= 10) {
			precision = 1;
		} else {
			precision = 2;
		}
		return QString("%1 dm³").arg(cubicDecimeters, 0, 'f', precision, '0');
	}

	double cubicCentimeters = cubicDecimeters * 1000;

	// display in cm³
	if (cubicCentimeters >= 100) {
		precision = 0;
	} else if (cubicCentimeters >= 10) {
		precision = 1;
	} else {
		precision = 2;
	}
	return QString("%1 cm³").arg(cubicCentimeters, 0, 'f', precision, '0');
}

QString UnitsHelper::toScientificNotation(double value) {
	auto exp = (int) floor(log10(value));
	const int precision = 2;
	return QString("%1 × 10<sup>%2</sup>")
	        .arg(value / pow(10, exp), 0, 'f', precision, '0')
	        .arg(exp);
}
