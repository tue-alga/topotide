#ifndef NETWORK_H
#define NETWORK_H

#include "striationcreator.h"

/**
 * A representative network of the river.
 *
 * The representative network consists of a number of paths from the source to
 * the sink.
 */
class Network {

	public:

		/**
		 * Creates an empty network, without paths.
		 */
		Network();

		/**
		 * A Morse-Smale edge in a network path.
		 *
		 * This is a separate class to be able to override `operator==` and
		 * `operator<`.
		 */
		struct PathEdge {

			/**
			 * The Morse-Smale edge.
			 */
			MsComplex::HalfEdge edge;

			/**
			 * Creates a new PathEdge.
			 * \param edge The Morse-Smale edge.
			 */
			PathEdge(MsComplex::HalfEdge edge) :
			    edge(edge) {
			}

			/**
			 * Checks if this edge is equally high to some other edge.
			 *
			 * \param other The other edge to check for.
			 * \return `true` if both edges are equally high; `false`
			 * otherwise.
			 */
			bool operator==(const PathEdge& other) const {
				Point p11 = edge.origin().data().p;
				Point p12 = edge.destination().data().p;
				Point highest1 = std::max(p11, p12);
				Point lowest1 = std::min(p11, p12);

				Point p21 = other.edge.origin().data().p;
				Point p22 = other.edge.destination().data().p;
				Point highest2 = std::max(p21, p22);
				Point lowest2 = std::min(p21, p22);

				return highest1 == highest2 && lowest1 == lowest2;
			}

			/**
			 * Checks if this edge is lower than some other edge.
			 *
			 * Edge _e_ is lower than edge _e'_ if
			 *
			 * * _e_'s highest endpoint is lower than _e'_'s highest endpoint,
			 *   or
			 * * _e_'s and _e'_'s highest endpoints are equally high, but
			 *   _e_'s lowest endpoint is lower than _e'_'s lowest endpoint.
			 *
			 * \param other The other edge to check for.
			 * \return `true` if this edge is lower than `other`; `false`
			 * otherwise.
			 */
			bool operator<(const PathEdge& other) const {
				Point p11 = edge.origin().data().p;
				Point p12 = edge.destination().data().p;
				Point highest1 = std::max(p11, p12);
				Point lowest1 = std::min(p11, p12);

				Point p21 = other.edge.origin().data().p;
				Point p22 = other.edge.destination().data().p;
				Point highest2 = std::max(p21, p22);
				Point lowest2 = std::min(p21, p22);

				// TODO this does not take edge length into account
				return highest1 < highest2
				        || (highest1 == highest2 && lowest1 < lowest2);
			}
		};

		/**
		 * A path from source to sink in a representative network.
		 */
		class Path {

			public:

				/**
				 * Creates a new Path.
				 *
				 * \param topToBottomOrder The ID of the path.
				 * \param edges A list of edges in the path, sorted from the
				 * source from the sink.
				 */
				Path(int topBottomOrder, const std::vector<PathEdge>& edges);

				/**
				 * Returns the edges in this path, in order from the source to
				 * the sink.
				 *
				 * \return The edges in this path.
				 */
				const std::vector<PathEdge>& edges() const;

				/**
				 * Returns the edges in this path, in order from the lowest to
				 * the highest edge.
				 *
				 * \return The edges in this path.
				 */
				const std::vector<PathEdge>& edgesOnHeight() const;

				/**
				 * Returns the index of this path in the top-to-bottom ordering
				 * of paths.
				 *
				 * \return The index of this path in the top-to-bottom ordering.
				 */
				int topBottomOrder() const;

				/**
				 * Checks whether some other path is lower than this path.
				 *
				 * This is defined by a lexicographic order. So, path _p_ is
				 * lower than path _q_ iff:
				 *
				 * * _p_'s highest edge is lower than _q_'s highest edge, or
				 * * _p_'s highest edge is equally high as _q_'s highest edge,
				 *   but _p_'s second-highest edge is lower than _q_'s
				 *   second-highest edge, or
				 * * ... _and so on_ ...
				 * * or all edge heights are equal, but _p_ contains fewer edges
				 *   than _q_ (in other words, there are edges in _q_ that
				 *   cannot be compared anymore to edges in _p_).
				 *
				 * \param other The path to compare this path to.
				 * \return `true` if this path is lower than `other`; `false`
				 * otherwise.
				 */
				bool operator<(const Path& other) const;

			private:

				/**
				 * The index of this path in a top-to-bottom ordering of all
				 * paths in the striation.
				 */
				int m_topBottomOrder;

				/**
				 * A list of edges in the path, in order from the source to the
				 * sink.
				 */
				std::vector<PathEdge> m_edges;

				/**
				 * A list of edges in the path, in order from the lowest to the
				 * highest edge.
				 */
				std::vector<PathEdge> m_edgesOnHeight;
		};

		/**
		 * Adds a path to the representative network.
		 * \param path The path to add.
		 */
		void addPath(const Path& path);

		/**
		 * Returns the list of paths in this network.
		 * \return A list of paths in this network.
		 */
		const std::vector<Path>& paths();

		/**
		 * Sorts the paths in the network from top to bottom.
		 */
		void sortTopToBottom();

	private:
		std::vector<Path> m_paths;
};

#endif // NETWORK_H
