#ifndef BOUNDARYREADER_H
#define BOUNDARYREADER_H

#include <QString>

#include <unordered_set>

#include "boundary.h"

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
		static Boundary readBoundary(
		        const QString& fileName, const HeightMap& map, QString& error);

	private:
		static void readPath(Path& path, const QStringList& numbers,
		              int length, const HeightMap& map, int& index);
};

#endif // BOUNDARYREADER_H
