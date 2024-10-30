#ifndef BOUNDARY_H
#define BOUNDARY_H

#include "heightmap.h"
#include "path.h"

/**
 * The boundary of the river area. It contains of four parts, which
 * are each represented by a Path. In order, these are:
 *
 * * the source,
 * * the top side,
 * * the sink, and
 * * the bottom side.
 *
 * The boundary can be either in clockwise or counter-clockwise order.
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
		 * Constructs a boundary with the given path components.
		 *
		 * \param source The source path.
		 * \param top The top path.
		 * \param sink The sink path.
		 * \param bottom The bottom path.
		 */
		Boundary(Path source, Path top, Path sink, Path bottom);

		/**
		 * The source path.
		 */
		Path m_source;

		/**
		 * The top path.
		 */
		Path m_top;

		/**
		 * The sink path.
		 */
		Path m_sink;

		/**
		 * The bottom path.
		 */
		Path m_bottom;

		/**
		 * Returns a new boundary that approximates this boundary, which is made
		 * up of edges of the heightmap. This method uses `Path::rasterize()` on
		 * the four boundary paths, and then removes common begin and end points
		 * of the paths.
		 */
		Boundary rasterize() const;

		/**
		 * Moves the last vertex of each component of the boundary so that it
		 * coincides with the first vertex of the next path. This is designed
		 * to be called after the user has dragged a vertex: as long as the
		 * code handling that ensures that the last vertex of a component is
		 * never draggable, this method fixes up the non-connectedness of the
		 * boundary.
		 */
		void ensureConnection();

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

		/**
		 * Checks if the boundary is in clockwise order.
		 */
		bool isClockwise() const;
};

#endif // BOUNDARY_H
