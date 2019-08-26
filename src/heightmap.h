#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include <vector>

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
				Coordinate(int x, int y) : m_x(x), m_y(y) {};
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
		 * Path through a heightmap.
		 */
		class Path {
			public:
				std::vector<Coordinate> m_points;

				void removeSpikes();
		};

		/**
		 * The boundary of the river area. It contains of four parts, which
		 * are each represented by a Path:
		 *
		 * * the source,
		 * * the top side,
		 * * the sink, and
		 * * the bottom side.
		 */
		class Boundary {
			public:
				Boundary();
				Boundary(Path source, Path top, Path sink, Path bottom);

				Path source;
				Path top;
				Path sink;
				Path bottom;
		};

		/**
		 * Returns the default boundary for this heightmap, that is, one that
		 * has the source on the left side and the sink on the right side, and
		 * spans the entire river.
		 *
		 * \return The boundary.
		 */
		Boundary defaultBoundary() const;

		/**
		 * Returns a Path along the top of the heightmap, from left to right.
		 * \return The top path.
		 */
		Path top() const;

		/**
		 * Returns a Path along the bottom of the heightmap, from left to right.
		 * \return The bottom path.
		 */
		Path bottom() const;

		/**
		 * Returns a Path along the left of the heightmap, from top to bottom.
		 * \return The left path.
		 */
		Path left() const;

		/**
		 * Returns a Path along the right of the heightmap, from top to bottom.
		 * \return The right path.
		 */
		Path right() const;

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
