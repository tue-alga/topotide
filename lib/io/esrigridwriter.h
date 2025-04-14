#ifndef ESRIGRIDWRITER_H
#define ESRIGRIDWRITER_H

#include <QString>

#include "../heightmap.h"
#include "../units.h"

/**
 * Class that handles writing an ESRI grid file (a.k.a. ASCII GRID).
 */
class EsriGridWriter {

	public:

		/**
		 * Writes an ESRI grid file.
		 *
		 * \param heigntMap The heightmap to output.
		 * \param fileName The file name of the grid file.
		 * \param units Reference to a Units object to read the units from. If
		 * there was a syntax error, this Units object is unchanged.
		 */
		static void writeGridFile(const HeightMap& heightMap, const QString& fileName,
		                          const Units& units);
};

#endif // ESRIGRIDWRITER_H
