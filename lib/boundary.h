#ifndef BOUNDARY_H
#define BOUNDARY_H

#include "heightmap.h"
#include "path.h"

/**
 * The boundary of the river area.
 */
class Boundary {

	public:

		/**
		 * Constructs an empty boundary.
		 */
		Boundary();

		/**
		 * Constructs the default boundary for the given size, that is, one
		 * that has the source on the left side and the sink on the right side,
		 * and spans the entire river.
		 */
		Boundary(int width, int height);

		/**
		 * Constructs the default boundary for the size of the given map.
		 */
		Boundary(const HeightMap& map);

		/**
		 * Constructs a boundary with the given path and no permeable regions.
		 */
		Boundary(Path path);

		/// Returns the path this boundary consists of.
		const Path& path() const;

		/// Moves the point at the given index to the given coordinate. If index
		/// points at the first or last point, this makes sure that the boundary
		/// stays closed by moving the last or first point too, respectively.
		void movePoint(int index, HeightMap::Coordinate c);
		/// Inserts a point at the given index at the given coordinate.
		void insertPoint(int index, HeightMap::Coordinate c);

		/**
		 * A part of the boundary. \ref m_start is considered to be in clockwise
		 * order before \ref m_end.
		 */
		struct Region {
			/// Index of the first vertex of the boundary which is in the
			/// region.
			int m_start;
			/// Index of the last vertex of the boundary which is in the
			/// region.
			int m_end;
		};

		/**
		 * Adds a permeable region to the boundary. Throws if the newly added
		 * region overlaps with an existing one.
		 */
		void addPermeableRegion(Region region);
		void setLastPermeableRegion(Region region);
		Region lastPermeableRegion() const;
		void removePermeableRegions();

		const std::vector<Region>& permeableRegions() const;
		std::vector<Region> impermeableRegions() const;

		/**
		 * Returns a new boundary that approximates this boundary, which is made
		 * up of edges of the heightmap. This method uses `Path::rasterize()` on
		 * the four boundary paths, and then removes common begin and end points
		 * of the paths.
		 */
		Boundary rasterize() const;

		/**
		 * Checks if the boundary is valid, that is, if it does not visit a
		 * coordinate more than once.
		 *
		 * Note that calling this method makes sense only after rasterizing,
		 * as it will not return `false` if two edges are crossing, but only if
		 * actual vertices coincide.
		 *
		 * \return `true` if each point is visited only at most once, `false`
		 * otherwise.
		 */
		bool isValid() const;

		static bool isClockwise(const Path& path);

	private:
		/**
		 * The path of this boundary, in clockwise order. Invariant: the start
		 * and end of the path are the same coordinate.
		 */
		Path m_path;

		/**
		 * The intervals of the boundary (on \ref m_path) which are to be
		 * considered permeable.
		 */
		std::vector<Region> m_permeableRegions;
};

#endif // BOUNDARY_H
