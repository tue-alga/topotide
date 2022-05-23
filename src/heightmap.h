#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include <vector>

#include <ostream>

#include <QImage>

/**
 * A two-dimensional heightmap that stores elevation data of a river.
 *
 * This is a wrapper around a QImage in which the elevation data is stored.
 * Every pixel in the QImage stores one elevation value. If _r_, _g_ and _b_ are
 * the red, green and blue components in [0, 256) of the color of the pixel,
 * respectively, then the pixel stores the elevation value
 * 256<sup>2</sup> * _r_ + 256 * _g_ + _b_. This means that the stored elevation
 * range is [0, 256<sup>3</sup>), that is, a heightmap stores elevation values
 * with 24 bits of vertical resolution; this should be enough for all practical
 * purposes.
 *
 * Note that this setup allows for greyscale images to be used as well,
 * although they only store 8 bits of elevation information.
 *
 * Storing heightmaps in images like this enables us to save them to ordinary
 * image files, which is useful for viewing and manipulating them.
 */
class HeightMap {

	public:

		/**
		 * Constructs an empty heightmap with width and height 0.
		 */
		HeightMap();

		/**
		 * Constructs a new heightmap from the given image.
		 * \param image The image to construct a heightmap from.
		 */
		HeightMap(QImage image);

		/**
		 * Returns the elevation at the given point.
		 *
		 * Assumes that `isInBounds(x, y)`.
		 *
		 * \param x The x-coordinate.
		 * \param y The y-coordinate.
		 * \return The elevation, in the range [0, 256<sup>3</sup>).
		 */
		int elevationAt(int x, int y) const;

		/**
		 * Returns the width of this heightmap.
		 */
		int width() const;

		/**
		 * Returns the height of this heightmap.
		 */
		int height() const;

		/**
		 * Coordinate of one height measurement in the heightmap.
		 */
		class Coordinate {

			public:

				/**
				 * The x-coordinate.
				 */
				int m_x;

				/**
				 * The y-coordinate.
				 */
				int m_y;

				/**
				 * Creates a coordinate.
				 *
				 * \param x The x-coordinate.
				 * \param y The y-coordinate.
				 */
				Coordinate(int x, int y) : m_x(x), m_y(y) {}

				friend std::ostream& operator<<(std::ostream& os,
				                                Coordinate const& c) {
					os << "(" << c.m_x << ", "
					   << c.m_y << ")";
					return os;
				}

				/**
				 * Returns the midpoint between the two given coordinates.
				 *
				 * The coordinates of the midpoint are rounded down to the
				 * nearest integer, if necessary.
				 *
				 * \param c1 The first coordinate.
				 * \param c2 The second coordinate.
				 * \return The midpoint.
				 */
				static Coordinate midpointBetween(Coordinate c1, Coordinate c2);
		};

		/**
		 * Checks whether the given point lies within the bounds of this
		 * heightmap.
		 *
		 * \param x The x-coordinate.
		 * \param y The y-coordinate.
		 * \return Whether `(x, y)` lies within this heightmap.
		 */
		bool isInBounds(int x, int y) const;

		/**
		 * Checks whether the given point lies within the bounds of this
		 * heightmap.
		 *
		 * \param c The coordinate.
		 * \return Whether \c c lies within this heightmap.
		 */
		bool isInBounds(Coordinate c) const;

		/**
		 * Returns the image corresponding to this heightmap.
		 * \return The image.
		 */
		QImage image() const;

		/**
		 * Returns the coordinate corresponding to the top-left point of this
		 * heightmap.
		 * \return The top-left coordinate.
		 */
		Coordinate topLeft() const;

		/**
		 * Returns the coordinate corresponding to the top-right point of this
		 * heightmap.
		 * \return The top-right coordinate.
		 */
		Coordinate topRight() const;

		/**
		 * Returns the coordinate corresponding to the bottom-left point of this
		 * heightmap.
		 * \return The bottom-left coordinate.
		 */
		Coordinate bottomLeft() const;

		/**
		 * Returns the coordinate corresponding to the bottom-right point of this
		 * heightmap.
		 * \return The bottom-right coordinate.
		 */
		Coordinate bottomRight() const;

	private:

		/**
		 * The QImage that stores the height data.
		 */
		QImage _image;
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
