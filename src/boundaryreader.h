#ifndef BOUNDARYREADER_H
#define BOUNDARYREADER_H

#include <QString>

#include <unordered_set>

#include "heightmap.h"

/**
 * Class that handles reading a text file containing a river boundary, and
 * transforming it into a HeightMap::Boundary.
 */
class BoundaryReader {

	public:

		/**
		 * Reads a boundary file and outputs the corresponding boundary.
		 *
		 * \param fileName The file name of the boundary file.
		 * \param error Reference to a QString to store an error message, in
		 * case there is a syntax error in the boundary file.
		 * \param map The river height map. This is used to determine if the
		 * boundary coordinates go out of bounds.
		 * \return The resulting boundary. If there was a syntax error, this
		 * returns an empty boundary.
		 */
		static HeightMap::Boundary readBoundary(
		        const QString& fileName, const HeightMap& map, QString& error);

	private:
		static void readPath(HeightMap::Path& path, const QStringList& numbers,
		              int length, const HeightMap& map, int& index);
		class CoordinateHasher {
			public:
				std::size_t operator()(const HeightMap::Coordinate& c) const {
					return std::hash<int>()(c.m_x)
					        ^ (std::hash<int>()(c.m_y) << 1);
				}
		};
		static void checkNoDouble(const HeightMap::Path& path,
		                   std::unordered_set<HeightMap::Coordinate,
		                          CoordinateHasher>& visited);
};

#endif // BOUNDARYREADER_H
