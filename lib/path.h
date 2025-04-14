#ifndef PATH_H
#define PATH_H

#include <functional>
#include <limits>
#include <optional>

#include "heightmap.h"
#include "point.h"

/**
 * Path through a heightmap. A path is a list of points in the
 * heightmap.
 */
class Path {

	public:

		/**
		 * Ordered list of points in this path.
		 */
		std::vector<HeightMap::Coordinate> m_points;

		/**
		 * Creates an empty path.
		 */
		Path();

		/**
		 * Creates a new path containing of two points.
		 *
		 * \param start The first point.
		 * \param end The second point.
		 */
		Path(HeightMap::Coordinate start, HeightMap::Coordinate end);

		/**
		 * Returns the start point of this path.
		 * @return The first point.
		 */
		HeightMap::Coordinate& start();
		const HeightMap::Coordinate& start() const;

		/**
		 * Returns the end point of this path.
		 * @return The last point.
		 */
		HeightMap::Coordinate& end();
		const HeightMap::Coordinate& end() const;

		/**
		 * Adds a point to the end of this path.
		 * \param point The point to add.
		 */
		void addPoint(HeightMap::Coordinate point);

		/**
		 * Appends another path to this path.
		 *
		 * \param path The path to append.
		 * \note The first point of `path` needs to be identical to the last
		 * point of this path, otherwise behavior is undefined.
		 */
		void append(Path path);

		/**
		 * Returns the length (number of edges) of this path. If this is an
		 * empty path, the length is defined as `-1`.
		 *
		 * @return The length.
		 */
		int length() const;

		/**
		 * Returns a new path that approximates this path, which is made
		 * up of edges of the heightmap. In other words, this method
		 * interpolates any edges between points that are further than
		 * unit distance apart.
		 */
		Path rasterize() const;

		/**
		 * Iteratively removes any spikes from this path. A spike is a
		 * point which is surrounded by two times the same point on both
		 * sides, in other words, a part of the path that looks like
		 * B-A-B. This part then gets simplified to just B; A gets
		 * removed entirely.
		 *
		 * Duplicated subsequent points are removed as well (A-A -> A).
		 *
		 * Note that removing a spike can result in another spike, so
		 * spikes are removed iteratively until no spikes are left
		 * (C-B-A-B-C -> C-B-C -> C).
		 */
		void removeSpikes();

		/**
		 * Determines if this path is valid. A path is valid if and only
		 * if it does not use any point more than once.
		 *
		 * \return \code true if this path is valid; \code false
		 * otherwise.
		 */
		bool isValid();

		/// Returns the index of the point on this path that is closest to c. If
		/// no point is at most `distanceLimit` away from c, returns
		/// `std::nullopt'.
		std::optional<int> closestTo(HeightMap::Coordinate c,
		                             double distanceLimit = std::numeric_limits<double>::infinity()) const;
		/// Returns the index of the point on this path that is closest to p and
		/// is accepted by the given lambda. If no accepted point is at most
		/// `distanceLimit` away from p, returns `std::nullopt'.
		std::optional<int> closestTo(
		    Point p, double distanceLimit = std::numeric_limits<double>::infinity(),
		    std::function<bool(int)> accept = [](int) { return true; }) const;

	private:
		void appendRasterizedEdgeTo(HeightMap::Coordinate point);
};

#endif // PATH_H
