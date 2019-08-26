#ifndef TEXTFILEREADER_H
#define TEXTFILEREADER_H

#include <QImage>
#include <QString>

#include "../units.h"

/**
 * Class that handles reading a text file containing elevation data, and
 * transforming it into a QImage for the HeightMap to use.
 */
class TextFileReader {

	public:

		/**
		 * Reads a text file and outputs a corresponding river image.
		 *
		 * \param fileName The file name of the text file.
		 * \param error Reference to a QString to store an error message, in
		 * case there is a syntax error in the text file.
		 * \param units Reference to a Units object to store the units in. If
		 * there was a syntax error, this Units object is unchanged.
		 * \return The resulting QImage. If there was a syntax error, this
		 * results a null image (see QImage::isNull()).
		 */
		static QImage readTextFile(
		        const QString& fileName, QString& error, Units& units);
};

#endif // TEXTFILEREADER_H
