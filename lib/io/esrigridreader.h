#ifndef ESRIGRIDREADER_H
#define ESRIGRIDREADER_H

#include <unordered_map>

#include <QLocale>
#include <QString>

#include "../heightmap.h"
#include "../units.h"

/**
 * Class that handles reading an ESRI grid file (a.k.a. ASCII GRID),
 * transforming it into a QImage for the HeightMap to use.
 */
class EsriGridReader {

	public:

		/**
		 * Reads an ESRI grid file and outputs a corresponding river heightmap.
		 *
		 * \param fileName The file name of the grid file.
		 * \param error Reference to a QString to store an error message, in
		 * case there is a syntax error in the text file.
		 * \param units Reference to a Units object to store the units in. If
		 * there was a syntax error, this Units object is unchanged.
		 * \return The resulting heightmap. If there was a syntax error, this
		 * results a 0x0 heightmap.
		 */
		static HeightMap readGridFile(
		        const QString& fileName, QString& error, Units& units);

	private:
		static HeightMap readGridFile(
		        const QString& fileName, QString& error, Units& units, QLocale locale);

		using Header = std::unordered_map<QString, std::variant<int, double>>;

		/**
		 * Returns an integer from the header. Throws an exception if the key
		 * doesn't exist, or the value retrieved is not an integer.
		 */
	    static int getIntFromHeader(Header& header, QString key);
		/**
		 * Returns a positive integer from the header. Throws an exception if
		 * the key doesn't exist, or the value retrieved is not a positive
		 * integer.
		 */
	    static int getPositiveIntFromHeader(Header& header, QString key);
		/**
		 * Returns a numeric value from the header. Throws an exception if the
		 * key doesn't exist.
		 */
	    static double getNumberFromHeader(Header& header, QString key);
};

#endif // ESRIGRIDREADER_H
