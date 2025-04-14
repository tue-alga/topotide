#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include <limits>
#include <ostream>
#include <vector>

/// A two-dimensional heightmap that stores elevation data of a river.
///
/// Each elevation value is stored as a `double`. Values can be marked as nodata
/// by using NaN (see \ref nodata).
 
/* TODO move to RiverWidget
 * This is a wrapper around a QImage in which the elevation data is stored.
 * Every pixel in the QImage stores one elevation value. If _r_, _g_ and _b_ are
 * the red, green and blue components in [0, 256) of the color of the pixel,
 * respectively, then the pixel stores the elevation value
 * 256<sup>2</sup> * _r_ + 256 * _g_ + _b_. This means that the stored elevation
 * range is [0, 256<sup>3</sup>), that is, a heightmap stores elevation values
 * with 24 bits of vertical resolution; this should be enough for all practical
 * purposes.
 */
class HeightMap {

	public:
		/// The constant used to mark nodata values.
		///
		/// \warning As this is NaN, comparing values to this constant does not
		/// work (always returns false). Use `std::isnan` instead.
		static constexpr double nodata = std::numeric_limits<double>::quiet_NaN();
		
		/// Coordinate of one height measurement in the heightmap.
		class Coordinate {

			public:
				/// The x-coordinate.
				int m_x;
				/// The y-coordinate.
				int m_y;

				/// Creates a coordinate.
				Coordinate(int x, int y) : m_x(x), m_y(y) {}

				/// Outputs a string representation of this coordinate.
				friend std::ostream& operator<<(std::ostream& os,
				                                Coordinate const& c) {
					os << "(" << c.m_x << ", "
					   << c.m_y << ")";
					return os;
				}

				/// Returns the midpoint between the two given coordinates.
				///
				/// The coordinates of the midpoint are rounded down to the
				/// nearest integer, if necessary.
				static Coordinate midpointBetween(Coordinate c1, Coordinate c2);

				int squaredDistanceTo(Coordinate other) const;
		};

		/// Constructs an empty heightmap with width and height 0. Such an empty
		/// heightmap is generally used in TopoTide to express error states
		/// (e.g., couldn't read an input file). See \ref isEmpty().
		HeightMap();
		/// Constructs an empty heightmap with the given width and height, in
		/// which all values are \ref nodata.
		HeightMap(int width, int height);

		/// Returns the elevation at the given coordinate. Assumes that
		/// `isInBounds(x, y)`.
		double elevationAt(int x, int y) const;
		/// Returns the elevation at the given coordinate. Assumes that
		/// `isInBounds(c)`.
		double elevationAt(Coordinate c) const;

		/// Sets the elevation at the given coordinate. Assumes that
		/// `isInBounds(x, y)`.
		void setElevationAt(int x, int y, double elevation);
		/// Sets the elevation at the given coordinate. Assumes that
		/// `isInBounds(c)`.
		void setElevationAt(Coordinate c, double elevation);

		/// Returns the width of this heightmap.
		int width() const;
		/// Returns the height of this heightmap.
		int height() const;
		/// Checks whether this heightmap is empty.
		bool isEmpty() const;

		/// Checks whether the given coordinate lies within the bounds of this
		/// heightmap.
		bool isInBounds(int x, int y) const;
		/// Checks whether the given coordinate lies within the bounds of this
		/// heightmap.
		bool isInBounds(Coordinate c) const;
		/// Returns the closest in-bounds coordinate to the given (possibly
		/// out-of-bounds) coordinate.
		Coordinate clampToBounds(Coordinate c) const;

		/// Computes the lowest (non-nodata) elevation in this heightmap.
		double minimumElevation() const;
		/// Computes the highest (non-nodata) elevation in this heightmap.
		double maximumElevation() const;

		/// Returns the coordinate corresponding to the top-left point of
		/// this heightmap.
		Coordinate topLeft() const;
		/// Returns the coordinate corresponding to the top-right point of
		/// this heightmap.
		Coordinate topRight() const;
		/// Returns the coordinate corresponding to the bottom-left point of
		/// this heightmap.
		Coordinate bottomLeft() const;
		/// Returns the coordinate corresponding to the bottom-right point of
		/// this heightmap.
		Coordinate bottomRight() const;

	private:
		/// The width of this heightmap.
		int m_width;
		/// The height of this heightmap.
		int m_height;
		/// The height values, as a list of size `m_width * m_height`, in
		/// row-major order. The height value at (x, y) is stored at
		/// `m_data[m_width * y + x]`.
		std::vector<double> m_data;
};

inline bool
operator==(const HeightMap::Coordinate& lhs, const HeightMap::Coordinate& rhs) {
	return lhs.m_x == rhs.m_x && lhs.m_y == rhs.m_y;
}

inline bool
operator!=(const HeightMap::Coordinate& lhs, const HeightMap::Coordinate& rhs) {
	return !operator==(lhs, rhs);
}

#endif /* HEIGHTMAP_H */
