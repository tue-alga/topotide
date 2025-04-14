#ifndef POINT_H
#define POINT_H

#include <cmath>
#include <iostream>

/**
 * A point with a position and an elevation value.
 */
class Point {

	public:

		/**
		 * Creates a new point at (0, 0, 0).
		 */
		Point();

		/**
		 * Creates a new point.
		 *
		 * \param x The x-coordinate.
		 * \param y The y-coordinate.
		 * \param h The height value.
		 */
		Point(double x, double y, double h);

		/**
		 * The x-coordinate.
		 */
		double x;

		/**
		 * The y-coordinate.
		 */
		double y;

		/**
		 * The elevation value.
		 */
		double h;

		/**
		 * Returns the distance to some other point.
		 * \param p The other point.
		 */
		double distanceTo(Point p);

		/**
		 * Returns whether this point is within the given bounds.
		 */
		inline bool isInBounds(int width, int height) {
			return x >= 0 && y >= 0 && x < width && y < height;
		}

		/**
		 * Compares two neighbors with respect to edge steepness.
		 *
		 * \param p1 The first neighbor.
		 * \param p2 The second neighbor.
		 * \return Whether `p1 < p2` based on edge steepness.
		 */
		bool compareNeighbors(Point p1, Point p2);

		/**
		 * Adds another point, coordinate-wise, to this point.
		 *
		 * \param other The point to add to this point.
		 * \return This point.
		 */
		Point& operator+=(const Point& other) {
			x += other.x;
			y += other.y;
			h += other.h;
			return *this;
		}

		/**
		 * Subtracts another point, coordinate-wise, from this point.
		 *
		 * \param other The point to subtract from this point.
		 * \return This point.
		 */
		Point& operator-=(const Point& other) {
			x -= other.x;
			y -= other.y;
			h -= other.h;
			return *this;
		}

		/**
		 * Scales this point by a factor.
		 *
		 * \param factor The factor to scale with.
		 * \return This point.
		 */
		Point& operator*=(const double factor) {
			x *= factor;
			y *= factor;
			h *= factor;
			return *this;
		}

		/**
		 * Outputs a representation of this point to the given output stream.
		 *
		 * @param os The output stream.
		 * @param f The point to output.
		 * @return The output stream.
		 */
		friend std::ostream& operator<<(std::ostream& os, Point const& p) {
			os << "(" << p.x << ", " << p.y << ", " << p.h << ")";
			return os;
		}
};

/**
 * Checks whether two points are identical.
 *
 * Points are considered identical if their *x*- and *y*-coordinates match.
 *
 * \param lhs The first point.
 * \param rhs The second point.
 * \return \c true if both points are equal; \c false otherwise.
 */
inline bool operator==(const Point& lhs, const Point& rhs) {
	// just check for comparison on x and y coordinate
	return lhs.x == rhs.x && lhs.y == rhs.y;
}

/**
 * Checks whether two points are not identical.
 *
 * Points are considered identical if their *x*- and *y*-coordinates match.
 *
 * \param lhs The first point.
 * \param rhs The second point.
 * \return \c false if both points are equal; \c true otherwise.
 */
inline bool operator!=(const Point& lhs, const Point& rhs) {
	return !operator==(lhs, rhs);
}

/**
 * Checks if this point is lower than another point.
 *
 * Point *p* is lower than point *q* if *p* has a strictly lower elevation value
 * than *q*. To avoid distinct points being considered equal, we furthermore
 * implement *simulation of simplicity*. This means that we endow points with
 * equal elevation values an arbitrary but consistent ordering based on their
 * *x*- and *y*-coordinates:
 *
 * * if the heights are equal, we check if *p* has a lower *x*-coordinate than
 *   *q*; if so, we say that *p < q*;
 *
 * * if also the *x*-coordinates are equal, we do the same with the
 *   *y*-coordinates.
 *
 * \note We assume that there are no two points at the same position with
 * different height values; in that case both *p == q* and *p < q* could be
 * true.
 *
 * \param lhs The first point.
 * \param rhs The second point.
 * \return \c true if *p < q* according to the description above; \c false
 * otherwise.
 */
inline bool operator<(const Point& lhs, const Point& rhs) {
	// avoid nan weirdness (nan != nan, etc.)
	double a = std::isnan(lhs.h) ? std::numeric_limits<double>::infinity() : lhs.h;
	double b = std::isnan(rhs.h) ? std::numeric_limits<double>::infinity() : rhs.h;
	// lexicographic comparison; this avoids degenerate cases where several
	// points have the same height
	return (a < b)
			|| (a == b && lhs.x < rhs.x)
			|| (a == b && lhs.x == rhs.x && lhs.y < rhs.y);
}

/**
 * Checks if this point is higher than another point.
 *
 * \see operator<(const Point&, const Point&)
 *
 * \param lhs The first point.
 * \param rhs The second point.
 * \return \c true if *p > q*; \c false otherwise.
 */
inline bool operator>(const Point& lhs, const Point& rhs) {
	return operator<(rhs, lhs);
}

/**
 * Checks if this point is lower than or equal to another point.
 *
 * \see operator<(const Point&, const Point&)
 *
 * \param lhs The first point.
 * \param rhs The second point.
 * \return \c true if *p <= q*; \c false otherwise.
 */
inline bool operator<=(const Point& lhs, const Point& rhs) {
	return !operator>(lhs, rhs);
}

/**
 * Checks if this point is higher than or equal to another point.
 *
 * \see operator<(const Point&, const Point&)
 *
 * \param lhs The first point.
 * \param rhs The second point.
 * \return \c true if *p >= q*; \c false otherwise.
 */
inline bool operator>=(const Point& lhs, const Point& rhs) {
	return !operator<(lhs, rhs);
}

inline Point operator+(Point lhs, const Point& rhs) {
	lhs += rhs;
	return lhs;
}

inline Point operator-(Point lhs, const Point& rhs) {
	lhs -= rhs;
	return lhs;
}

inline Point operator*(Point lhs, double factor) {
	lhs *= factor;
	return lhs;
}

#endif /* POINT_H */
