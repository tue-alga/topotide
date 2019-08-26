#ifndef UNITS_H
#define UNITS_H

#include "point.h"

/**
 * Converter between our internal units (elevations between 0 and 0xffffff;
 * x- and y-values in pixel counts) and real-world units.
 */
class Units {

	public:

		/**
		 * Creates a default unit converter, that is, one with x- and
		 * y-resolution 1, minimum elevation 0 and maximum elevation 10.
		 */
		Units();

		/**
		 * Creates a unit converter with the given parameters.
		 *
		 * \param xResolution The horizontal resolution (in meters per pixel)
		 * in the x-direction.
		 * \param yResolution The horizontal resolution (in meters per pixel)
		 * in the y-direction.
		 * \param minElevation The minimum elevation.
		 * \param maxElevation The maximum elevation.
		 *
		 * \note \c xResolution and \c yResolution must be positive, and \c
		 * minElevation must be strictly smaller than \c maxElevation; otherwise
		 * behavior is undefined.
		 */
		Units(double xResolution,
		      double yResolution,
		      double minElevation,
		      double maxElevation);

		/**
		 * Computes the two-dimensional length in meters of a line segment,
		 * specified in internal coordinates.
		 *
		 * \param p1 The first point.
		 * \param p2 The second point.
		 * \return The length of the line segment `p1 -> p2`.
		 */
		double length(Point p1, Point p2) const;

		/**
		 * Computes the two-dimensional length in meters of a line segment,
		 * specified in internal coordinates.
		 *
		 * \param x1 The x-coordinate of the first point.
		 * \param y1 The y-coordinate of the first point.
		 * \param x2 The x-coordinate of the second point.
		 * \param y2 The y-coordinate of the second point.
		 * \return The length of the line segment `(x1, y1) -> (x2, y2)`.
		 */
		double length(int x1, int y1,
		              int x2, int y2) const;

		/**
		 * Converts an internal elevation to a real-world height value.
		 *
		 * \param elevation The interval elevation value.
		 * \return The real-world height value.
		 */
		double toRealElevation(int elevation) const;

		/**
		 * Computes the volume in cubic meters, given a volume in internal
		 * coordinates.
		 *
		 * In internal coordinates, a volume of 2^24 corresponds to the volume
		 * of a 1x1 pixel rectangle over the entire height range.
		 *
		 * \param volume The volume in internal coordinates.
		 * \return The volume in cubic meters.
		 */
		double toRealVolume(double volume) const;

		/**
		 * Computes the volume in internal coordinates, given a volume in cubic
		 * meters.
		 *
		 * This is the inverse of toRealVolume().
		 *
		 * \param volume The volume in cubic meters.
		 * \return The volume in internal coordinates.
		 */
		double fromRealVolume(double volume) const;

		/**
		 * The horizontal resolution in the x-direction, in meters per pixel.
		 */
		double m_xResolution;

		/**
		 * The horizontal resolution in the y-direction, in meters per pixel.
		 */
		double m_yResolution;

		/**
		 * The elevation in meters that corresponds to pure black (value 0).
		 */
		double m_minElevation;

		/**
		 * The elevation in meters that corresponds to pure white (value 2^24).
		 */
		double m_maxElevation;
};

#endif // UNITS_H
