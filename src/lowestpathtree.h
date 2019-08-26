#ifndef LOWESTPATHTREE_H
#define LOWESTPATHTREE_H

#include <vector>

#include "carvedcomplex.h"
#include "units.h"

/**
 * A lowest-path tree, that is, a data structure that stores, for any point in
 * a Morse-Smale complex, the lowest paths to the source and the sink.
 */
class LowestPathTree {

	public:

		/**
		 * Creates a lowest-path tree for the given Morse-Smale complex.
		 *
		 * \note Changing the Morse-Smale complex after calling this constructor
		 * invalidates the LowestPathTree.
		 *
		 * \param msc The Morse-Smale complex to build a lowest-path tree for.
		 * \param source The source vertex.
		 * \param sink The sink vertex.
		 * \param units Real-world units of the river. This is necessary to
		 * compute edge lengths.
		 */
		LowestPathTree(CarvedComplex* msc,
		               CarvedComplex::Vertex source,
		               CarvedComplex::Vertex sink,
		               Units units);

		/**
		 * Determines the lowest path from the given vertex to the source.
		 *
		 * \param from The vertex to start from.
		 * \return The lowest path.
		 */
		CarvedComplex::Path lowestPathToSource(CarvedComplex::Vertex from);

		/**
		 * Determines the lowest path from the given vertex to the sink.
		 *
		 * \param from The vertex to start from.
		 * \return The lowest path.
		 */
		CarvedComplex::Path lowestPathToSink(CarvedComplex::Vertex from);

	private:

		/**
		 * The Morse-Smale complex.
		 */
		CarvedComplex* msc;

		/**
		 * The source vertex.
		 */
		CarvedComplex::Vertex m_source;

		/**
		 * The sink vertex.
		 */
		CarvedComplex::Vertex m_sink;
};

#endif // LOWESTPATHTREE_H
