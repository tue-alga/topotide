#ifndef LINKSEQUENCE_H
#define LINKSEQUENCE_H

#include "networkgraph.h"

/**
 * A representation of a network as an ordered sequence of links.
 *
 * A *link* is a maximal sequence of edges in the network having the same
 * δ-value.
 */
class LinkSequence {

	public:

		/**
		 * Creates a link sequence from a NetworkGraph.
		 * \param graph The graph to convert.
		 */
		LinkSequence(const NetworkGraph& graph);

		/**
		 * One link in the sequence.
		 */
		struct Link {
			double delta;
			std::vector<Point> path;
		};

		/**
		 * Returns the number of links in this sequence.
		 * \return The link count.
		 */
		int linkCount();

		/**
		 * Returns a link from the sequence.
		 *
		 * \param id The index of the link to return.
		 * \return The link.
		 */
		Link& link(int id);

	private:
		std::vector<Link> m_links;

		/**
		 * Appends the path of the given edge to the link.
		 *
		 * This method assumes the link has a non-empty path.
		 *
		 * \param link The link to add to.
		 * \param graph The network graph.
		 * \param e The edge of which the path is to be added to the link.
		 */
		static void appendEdgeToLink(
		        Link& link, const NetworkGraph& graph,
		        const NetworkGraph::Edge& e);

		static int otherEndOf(const NetworkGraph::Edge& e, int oneEnd);
};

#endif // LINKSEQUENCE_H
