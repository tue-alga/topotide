#ifndef GRAPHWRITER_H
#define GRAPHWRITER_H

#include <QString>
#include <QTextStream>

#include "../networkgraph.h"
#include "../units.h"

/**
 * Writer that outputs graph files.
 */
class GraphWriter {

	public:

		/**
		 * Writes a river image to a graph text file.
		 *
		 * The format is as follows:
		 *
		 * ```
		 * <vertex-count>
		 * <id> <x> <y>  # for each vertex
		 * <edge-count>
		 * <id> <from-id> <to-id> <delta> (<x> <y>)*  # for each edge
		 * ```
		 *
		 * \param networkGraph The graph to output.
		 * \param units The unit converter, used to output the delta values
		 * in natural units.
		 * \param fileName The file name of the image file.
		 */
		static void writeGraph(NetworkGraph& networkGraph,
		                       const Units& units,
		                       const QString& fileName);
};

#endif // GRAPHWRITER_H
