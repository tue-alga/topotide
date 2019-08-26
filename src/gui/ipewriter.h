#ifndef IPEWRITER_H
#define IPEWRITER_H

#include <QString>
#include <QTextStream>

#include "../heightmap.h"
#include "../networkgraph.h"
#include "../units.h"

#ifdef WITH_IPELIB
#include <ipegeo.h>
#include <ipepath.h>

#include "../linksequence.h"
#endif

/**
 * Writer that outputs Ipe files.
 */
class IpeWriter {

	public:

		/**
		 * Writes a river image to an Ipe file.
		 *
		 * \param heightMap The river heightmap.
		 * \param networkGraph The generated river network.
		 * \param fileName The file name of the image file.
		 */
		static void writeIpeFile(const HeightMap& heightMap,
		                         const NetworkGraph& networkGraph,
		                         const Units& units,
		                         const QString& fileName);

#ifdef WITH_IPELIB
	private:

		/**
		 * Creates an Ipe path corresponding to some link in the link sequence.
		 *
		 * \param link The link to convert.
		 * \return The corresponding Ipe path.
		 */
		static ipe::Path* linkToIpePath(
		        const LinkSequence::Link& link, double deltaMax);

		static ipe::Color toIpeColor(QColor color);

		/**
		 * Creates an Ipe mark corresponding to a vertex.
		 *
		 * \param vertex The vertex to convert.
		 * \return The corresponding Ipe mark.
		 */
		static ipe::Reference* toIpeMark(const NetworkGraph::Vertex& v);

		static double m_verticalStretch;
		static int m_height;

		static double scaleFactor;
#endif
};

#endif // IPEWRITER_H
