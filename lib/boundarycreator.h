#ifndef BOUNDARYCREATOR_H
#define BOUNDARYCREATOR_H

#include <QString>

#include <optional>

#include "boundary.h"
#include "heightmap.h"
#include "inputdcel.h"

/**
 * Class that allows constructing a boundary path from a contiguous area of
 * non-nodata values in the heightmap.
 *
 * To use this, first call the constructor with the heightmap to use. Then call
 * \ref setSeed() with the "seed" around which to construct the contiguous area.
 * Finally, you can retrieve the generated boundary path using \ref getPath().
 */
class BoundaryCreator {

	public:
		/// Creates a BoundaryCreator from the given heightmap.
		BoundaryCreator(HeightMap heightMap);

		/// Creates a BoundaryCreator from an already existing boundary, instead
		/// of by finding the reachable data area from a seed. This just lets
		/// you set new source and sink locations on the existing boundary
		/// instead of drawing a fully new boundary.
		///
		/// \note When using this constructor, do not call \ref
		/// setSeed(HeightMap::Coordinate).
		BoundaryCreator(const Boundary& b);

		/// Finds the reachable data area from the given seed.
		void setSeed(HeightMap::Coordinate seed);

		/// Returns the path around the data area this BoundaryCreator is
		/// working with. (Before calling \ref setSeed(HeightMap::Coordinate)
		/// this is `std::nullopt`, unless you used the \ref
		/// BoundaryCreator(const Boundary&) constructor.)
		std::optional<Path> getPath() const;

	private:
		HeightMap m_heightMap;
		InputDcel m_inputDcel;
		std::optional<Path> m_path;
};

#endif // BOUNDARYCREATOR_H
