#ifndef GRAPH_H
#define GRAPH_H

#include <vector>

#include "boundary.h"
#include "boundarystatus.h"
#include "heightmap.h"
#include "point.h"

/**
 * The initial graph that is created from the height map.
 *
 * The input graph consists of a vertex for every height value, a source
 * and a sink vertex, and a global maximum vertex.
 */
class InputGraph {

	public:

		/**
		 * An adjacency in a graph.
		 */
		struct Adjacency {

			/**
			 * Creates a new adjacency.
			 *
			 * \param from The ID of the origin vertex.
			 * \param to The ID of the destination vertex.
			 */
			Adjacency(int from, int to) : from(from), to(to) {
			}

			/**
			 * The ID of the origin vertex.
			 */
			int from;

			/**
			 * The ID of the destination vertex.
			 */
			int to;

			/// Whether this adjacency is on the boundary.
			BoundaryStatus boundaryStatus = BoundaryStatus::INTERIOR;

			/// When `boundaryStatus == BoundaryStatus::PERMEABLE`, this stores
			/// the index of that permeable region.
			std::optional<int> permeableRegion;
		};

		/**
		 * A vertex in a graph.
		 */
		struct Vertex {

			/**
			 * Creates a new vertex with the given ID.
			 *
			 * \param id The ID. Should be equal to the position in the vertices
			 * list of the graph.
			 */
			explicit Vertex(int id);

			/**
			 * The adjacent vertices.
			 *
			 * The adjacencies are put in this list in counter-clockwise order
			 * (if we assume that the y-coordinate increases in downwards
			 * direction).
			 */
			std::vector<Adjacency> adj;

			/**
			 * The ID of this vertex.
			 */
			int id;

			/**
			 * The position (x and y-coordinate and height value) of this
			 * vertex.
			 */
			Point p;

			/// Whether this vertex is on the boundary.
			BoundaryStatus boundaryStatus = BoundaryStatus::IMPERMEABLE;

			/// When `boundaryStatus == BoundaryStatus::PERMEABLE`, this stores the
			/// index of that permeable region.
			std::optional<int> permeableRegion;

			/// Finds the index of the adjacency to the given vertex, if it
			/// exists.
			std::optional<int> findAdjacencyTo(int to) const;

			/**
			 * Adds an adjacency to the back of the adjacency list of this
			 * vertex.
			 *
			 * The following calls are equivalent:
			 *
			 * ```
			 * v.addAdjacency(to);
			 * v.adj.push_back(Adjacency(v._id, to));
			 * ```
			 *
			 * \param to The destination vertex.
			 */
			void addAdjacencyAfter(int to);

			/**
			 * Adds an adjacency to the front of the adjacency list of this
			 * vertex.
			 *
			 * \param to The destination vertex.
			 */
			void addAdjacencyBefore(int to);
		};

		/**
		 * Creates an empty graph.
		 */
		InputGraph();

		/**
		 * Creates a graph corresponding to the given heightmap.
		 *
		 * \note The heightmap needs to have height at least 2; otherwise
		 * behavior is undefined.
		 *
		 * \param heightMap The heightmap.
		 */
		InputGraph(const HeightMap& heightMap);

		/**
		 * Creates a graph corresponding to the part of the given heightmap that
		 * is within the given boundary.
		 *
		 * \note The heightmap needs to have height at least 2; otherwise
		 * behavior is undefined.
		 *
		 * \param heightMap The heightmap.
		 * \param boundary The boundary. Everything inside this boundary is
		 * included in the graph.
		 */
	    InputGraph(const HeightMap& heightMap, Boundary boundary);

	    /**
		 * Returns the `i`th vertex in the graph.
		 *
		 * \param i The index.
		 * \return The vertex.
		 */
		Vertex& operator[](int i);

		/**
		 * Returns the `i`th vertex in the graph.
		 *
		 * \param i The index.
		 * \return The vertex.
		 */
		const Vertex& operator[](int i) const;

		/**
		 * Returns the number of vertices in the graph.
		 * \return The number of vertices.
		 */
		int vertexCount() const;

		/**
		 * Adds a new vertex.
		 * \return The ID of the new vertex.
		 */
		int addVertex();

		/**
		 * Returns the number of edges in the graph.
		 * \return The number of edges.
		 */
		int edgeCount() const;

		/**
		 * Removes all edges in the graph.
		 */
		void clearAllEdges();

		/**
		 * Checks whether the given adjacency is ascending or descending, as
		 * seen from the origin vertex.
		 *
		 * \param a The adjacency.
		 * \return `true` if `a` is ascending, `false` otherwise.
		 */
		bool isAscending(const Adjacency& a) const;

		/**
		 * Checks if the terrain contains nodata values.
		 */
		bool containsNodata() const;

	private:
		/**
		 * Adds a new vertex with the given point.
		 * \return The ID of the new vertex.
		 */
		int addVertex(Point p);

		/**
		 * Returns the (possible) 4 neighbors of the given point.
		 *
		 * \param v The point.
		 * \return A list of the 4 neighbors, in counter-clockwise order,
		 * starting from the edge going to the right.
		 */
		std::vector<HeightMap::Coordinate> neighborsOf(HeightMap::Coordinate v);

		/// Marks a vertex with the given boundary status and permeable region.
		void markVertex(HeightMap::Coordinate c, BoundaryStatus status,
					std::optional<int> permeableRegion = std::nullopt);
		/// Marks an edge (i.e., both adjacencies representing that edge) with
		/// the given boundary status and permeable region.
		void markEdge(HeightMap::Coordinate c1, HeightMap::Coordinate c2, BoundaryStatus status,
					std::optional<int> permeableRegion = std::nullopt);

		/**
		 * List of the vertices.
		 */
		std::vector<Vertex> m_verts;
	
		/// Mapping from HeightMap coordinates to vertex IDs. m_vertexMap[x][y]
		/// is the index of the InputGraph vertex representing this HeightMap
		/// coordinate.
		std::vector<std::vector<int>> m_vertexMap;
};

// comparison operators for Adjacency

inline bool operator==(const InputGraph::Adjacency& lhs,
                       const InputGraph::Adjacency& rhs) {
	return lhs.from == rhs.from && lhs.to == rhs.to;
}

inline bool operator!=(const InputGraph::Adjacency& lhs,
                       const InputGraph::Adjacency& rhs) {
	return !operator==(lhs, rhs);
}

// comparison operators for Vertex

inline bool operator<(const InputGraph::Vertex& lhs,
                      const InputGraph::Vertex& rhs) {
	// lexicographic comparison; this avoids degenerate cases where several
	// points have the same height
	return lhs.p.h < rhs.p.h
	        || (lhs.p.h == rhs.p.h && lhs.p.x < rhs.p.x)
	        || (lhs.p.h == rhs.p.h && lhs.p.x == rhs.p.x && lhs.p.y < rhs.p.y);
}

inline bool operator>(const InputGraph::Vertex& lhs,
                      const InputGraph::Vertex& rhs) {
	return  operator<(rhs, lhs);
}

inline bool operator<=(const InputGraph::Vertex& lhs,
                       const InputGraph::Vertex& rhs) {
	return !operator>(lhs, rhs);
}

inline bool operator>=(const InputGraph::Vertex& lhs,
                       const InputGraph::Vertex& rhs) {
	return !operator<(lhs, rhs);
}

#endif /* GRAPH_H */
