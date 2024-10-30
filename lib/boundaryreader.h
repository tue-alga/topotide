#ifndef BOUNDARYREADER_H
#define BOUNDARYREADER_H

#include <QString>

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
		 * \param width The width of the height map. This is used to determine
		 * if the boundary coordinates go out of bounds.
		 * \param height The height of the height map. This is used to determine
		 * if the boundary coordinates go out of bounds.
		 * \return The resulting boundary. If there was a syntax error, this
		 * returns an empty boundary.
		 */
		static Boundary readBoundary(
		        const QString& fileName, int width, int height, QString& error);

	private:
		static void readPath(Path& path, const QStringList& numbers,
		              int length, int width, int height, int& index);
};

#endif // BOUNDARYREADER_H
