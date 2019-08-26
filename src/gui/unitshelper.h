#ifndef UNITSHELPER_H
#define UNITSHELPER_H

#include <QString>

/**
 * Several helper functions that produce human-readable representations of
 * units.
 */
class UnitsHelper {

	public:

		/**
		 * Produces a human-readable representation of the given elevation
		 * value.
		 *
		 * \param meters The elevation value, in meters.
		 * \return The resulting text.
		 */
		static QString formatElevation(double meters);

		/**
		 * Produces a human-readable representation of the given volume value.
		 *
		 * \param cubicMeters The volume value, in cubic meters.
		 * \return The resulting text.
		 */
		static QString formatVolume(double cubicMeters);

	private:
		static QString toScientificNotation(double value);
};

#endif // UNITSHELPER_H
