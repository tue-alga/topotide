#ifndef LINKSEQUENCEWRITER_H
#define LINKSEQUENCEWRITER_H

#include <QString>
#include <QTextStream>

#include "../linksequence.h"
#include "../units.h"

/**
 * Writer that outputs link sequence files.
 */
class LinkSequenceWriter {

	public:

		/**
		 * Writes a link sequence to a text file.
		 *
		 * \param linkSequence The link sequence to output.
		 * \param units The unit converter, used to output the delta values
		 * in natural units.
		 * \param fileName The file name of the image file.
		 */
		static void writeLinkSequence(LinkSequence& linkSequence,
		                              const Units& units,
		                              const QString& fileName);
};

#endif // LINKSEQUENCEWRITER_H
