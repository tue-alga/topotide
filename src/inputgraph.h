#ifndef GRAPH_H
#define GRAPH_H

#include <vector>

#include "heightmap.h"
#include "point.h"
#include "units.h"
#include "vertextype.h"

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
			Adjacency(int from, int to) :
			    from(from), to(to), disambiguation(false) {
			}

			/**
			 * Creates a new adjacency.
			 *
			 * \param from The ID of the origin vertex.
			 * \param to The ID of the destination vertex.
			 * \param disambiguation The disambiguation flag.
			 */
			Adjacency(int from, int to, bool disambiguation) :
			    from(from), to(to), disambiguation(disambiguation) {
			}

			/**
			 * The ID of the origin vertex.
			 */
			int from;

			/**
			 * The ID of the destination vertex.
			 */
			int to;

			/**
			 * A flag used to disambiguate two adjacencies between the same
			 * origin and destination.
			 *
			 * If there is only one source (sink) then it needs two connections
			 * to the global maximum. When creating an InputDcel from this
			 * InputGraph, we need a way to tell these two adjacencies apart.
			 * Therefore, we set this flag to true for all adjacencies between
			 * the bottom path and the global maximum. Then in the InputDcel
			 * construction we regard two adjacencies as equal only if their
			 * \c disambiguation flags are the same.
			 */
			bool disambiguation;
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
			Vertex(int id);

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
			void addAdjacency(int to, bool disambiguation = false);
		};

		/**
		 * Creates an empty graph.
		 *
		 * \param units The real-world units. Used for finding steepest-descent
		 * paths.
		 */
		InputGraph(Units units = Units());

		/**
		 * Creates a graph corresponding to the given heightmap.
		 *
		 * \note The heightmap needs to have height at least 2; otherwise
		 * behavior is undefined.
		 *
		 * \param heightMap The heightmap.
		 * \param units The real-world units. Used for finding steepest-descent
		 * paths.
		 */
		InputGraph(const HeightMap& heightMap, Units units = Units());

		InputGraph(const HeightMap& heightMap,
		           HeightMap::Path top,
		           HeightMap::Path bottom,
		           HeightMap::Path source,
		           HeightMap::Path sink,
		           Units units = Units());

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
		 * Returns the vertex type of a vertex.
		 *
		 * \param i The ID of the vertex.
		 * \return The vertex type.
		 */
		VertexType vertexType(int i) const;

		/**
		 * The wedges of a vertex.
		 */
		struct Wedges {

			/**
			 * List of the steepest-ascent neighbors in each ascending wedge,
			 * as indices in the adj array.
			 */
			std::vector<int> ascending;

			/**
			 * List of the steepest-descent neighbors in each descending wedge,
			 * as indices in the adj array.
			 */
			std::vector<int> descending;
		};

		/**
		 * Returns the ascending and descending wedges of a vertex.
		 *
		 * \param i The ID of the vertex.
		 * \return The ascending and descending wedges of this vertex.
		 *
		 * \note This method assumes that the vertex has at least one neighbor.
		 * If it does not, behavior is undefined.
		 */
		Wedges getWedges(int i) const;

		/**
		 * Returns the steepest-descending edge from a vertex.
		 *
		 * \param i The ID of the vertex.
		 * \return The index of the steepest-descending edge from `i` in its
		 * adjacency list, or `-1` if this vertex does not have any descending
		 * edges.
		 */
		int steepestDescentFrom(int i) const;

		/**
		 * Checks if `a2` is steeper-ascending than `a1`. Or, alternatively, if
		 * `a1` is steeper-descending than `a2`.
		 *
		 * \param a1 The first adjacency.
		 * \param a2 The second adjacency.
		 * \return Whether `a2` is steeper than `a1`.
		 */
		bool compareSteepness(const Adjacency& a1, const Adjacency& a2) const;

		/**
		 * Returns the length of the given adjacency.
		 *
		 * \param a The adjacency.
		 * \return Its length.
		 */
		double adjacencyLength(const Adjacency& a) const;

	private:

		/**
		 * Adds the neighbor to the given vertex, but only if it is in bounds;
		 * otherwise adds a neighbor to the source or sink (if the x-coordinate
		 * is out of bounds).
		 *
		 * \param heightMap The height map.
		 * \param v The ID of the current vertex.
		 * \param source The ID of the source.
		 * \param sink The ID of the sink.
		 * \param globalMaximum The ID of the globalMaximum.
		 * \param nx The x-coordinate of the proposed neighbor.
		 * \param ny The y-coordinate of the proposed neighbor.
		 *
		 * \note This is an auxiliary method for `Graph(HeightMap)`.
		 */
		void addNeighbor(const HeightMap& heightMap, int v,
				int source, int sink, int globalMaximum,
				int nx, int ny);

		/**
		 * Returns the (possible) 6 neighbors of the given point.
		 *
		 * \param v The point.
		 * \return A list of the 6 neighbors, in counter-clockwise order,
		 * starting from the edge going to the right.
		 */
		std::vector<HeightMap::Coordinate> neighborsOf(HeightMap::Coordinate v);

		/**
		 * List of the vertices.
		 */
		std::vector<Vertex> m_verts;

		/**
		 * The real-world units.
		 */
		Units m_units;
};

// comparison operators for Adjacency

inline bool operator==(const InputGraph::Adjacency& lhs,
                       const InputGraph::Adjacency& rhs) {
	return lhs.from == rhs.from &&
	        lhs.to == rhs.to &&
	        lhs.disambiguation == rhs.disambiguation;
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
