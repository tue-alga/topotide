#ifndef BOUNDARYWRITER_H
#define BOUNDARYWRITER_H

#include <QString>
#include <QTextStream>

#include "boundary.h"

/**
 * Writer that outputs boundary files.
 */
class BoundaryWriter {

	public:

		/**
		 * Writes a boundary to a text file.
		 *
		 * \param boundary The boundary to output.
		 * \param fileName The file name of the image file.
		 */
		static void writeBoundary(Boundary& boundary, const QString& fileName);

	private:
		static int regionLength(const Path& path, int start, int end);
		static void writeRegion(QTextStream& out, const Path& path, int start, int end);
};

#endif // BOUNDARYWRITER_H
