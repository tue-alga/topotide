#ifndef NETWORKGRAPHCREATOR_H
#define NETWORKGRAPHCREATOR_H

#include <functional>

#include "mscomplex.h"
#include "network.h"
#include "networkgraph.h"

/**
 * An algorithm for converting a network (set of source-sink paths) into a
 * graph structure.
 *
 * The idea is to mark all (directed) half-edges used by paths in the network.
 * Then we do a DFS, starting from the source, to construct the graph. In this
 * DFS all vertices of degree 2 are ignored.
 *
 * \note This is separate from the NetworkGraph class itself, so it is
 * possible to obtain progress information while the graph is being computed.
 */
class NetworkGraphCreator {

	public:

		/**
		 * Creates a network graph creator that can create a graph from a
		 * network.
		 *
		 * \note Call create() to actually execute the algorithm.
		 *
		 * \param msc The Morse-Smale complex of the river.
		 * \param network The network to convert.
		 * \param networkGraph An empty network graph to store the result in.
		 * \param simplify Whether to simplify the graph.
		 * \param progressListener A function that is called when a progress
		 * update is available.
		 */
		NetworkGraphCreator(
		        MsComplex& msc,
		        Network& network,
		        NetworkGraph* networkGraph,
		        bool simplify,
		        std::function<void(int)> progressListener = nullptr);

		/**
		 * Creates the representative network.
		 */
		void create();

	private:

		/**
		 * The Morse-Smale complex.
		 */
		MsComplex& msc;

		/**
		 * The network to convert.
		 */
		Network& network;

		/**
		 * The graph that we are going to store our result in.
		 */
		NetworkGraph* networkGraph;

		/**
		 * Whether to simplify the graph.
		 */
		bool simplify;

		/**
		 * List that indicates, for every half-edge in `msc`, whether it is
		 * _marked_. A half-edge gets marked iff it is used in at least one
		 * network path.
		 */
		std::vector<bool> marked;

		/**
		 * Returns the next interesting vertex on the path starting from the
		 * given half-edge. Interesting vertices are defined as vertices that
		 * do not have exactly one incoming and one outgoing marked edge; the
		 * idea is that non-interesting vertices can be skipped.
		 *
		 * This method starts from the given half-edge and as long as it keeps
		 * encountering non-interesting vertices, moves to its outgoing marked
		 * half-edge.
		 *
		 * \image html network-graph-interesting.svg
		 *
		 * \param edge The half-edge to start from.
		 * \return The next interesting vertex on the path starting from `edge`.
		 *
		 * \todo Update documentation.
		 */
		std::vector<MsComplex::HalfEdge> nextInterestingVertex(
		        MsComplex::HalfEdge edge);

		/**
		 * Checks whether a vertex is boring. A vertex is boring if it has one
		 * incoming and one outgoing marked edge that are not twins, or if it
		 * has two twin-pairs of incoming and outgoing marked edges. The idea is
		 * that boring vertices can be simplified away.
		 *
		 * If `simplify` is set to `false`, this method always returns `false`,
		 * as we do not want to simplify any vertex away.
		 *
		 * \param v The vertex to check.
		 * \return Whether `v` is an boring vertex.
		 */
		bool isBoring(MsComplex::Vertex v);

		/**
		 * Given a half-edge, returns another marked outgoing half-edge.
		 *
		 * If there are several marked outgoing edges, this method returns an
		 * arbitrary one.
		 *
		 * \param e The half-edge.
		 * \return Another marked outgoing edge of `e.origin()`, or an
		 * uninitialized half-edge if no marked outgoing edge exists.
		 */
		MsComplex::HalfEdge otherMarkedOutgoingEdge(MsComplex::HalfEdge e);

		/**
		 * Calls the progress listener, if one is set.
		 * \param progress The progress value to report.
		 */
		void signalProgress(int progress);

		/**
		 * A function that is called when there is a progress update.
		 */
		std::function<void(int)> progressListener;
};

#endif // NETWORKGRAPHCREATOR_H
