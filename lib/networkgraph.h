#ifndef NETWORKGRAPH_H
#define NETWORKGRAPH_H

#include <vector>

#include "point.h"

/**
 * A directed graph structure for the computed representative network.
 */
class NetworkGraph {

	public:

		/**
		 * An edge in a graph.
		 */
		struct Edge {

			/**
			 * Creates a new edge.
			 *
			 * \param id The ID of this edge.
			 * \param from The ID of the origin vertex.
			 * \param to The ID of the destination vertex.
			 * \param path A list of points on the edge.
			 */
			Edge(int id, int from, int to, std::vector<Point> path) :
			    id(id), from(from), to(to), path(path) {
			}

			/**
			 * The ID of this edge.
			 */
			int id;

			/**
			 * The ID of the origin vertex.
			 */
			int from;

			/**
			 * The ID of the destination vertex.
			 */
			int to;

			/**
			 * A list of points on the path from the origin to the destination
			 * vertex.
			 */
			std::vector<Point> path;

			/**
			 * If applicable, the δ-value of this edge (see
			 * MsHalfEdge::m_delta).
			 */
			double delta;
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
			 * \param p The position of the vertex.
			 */
			Vertex(int id, Point p);

			/**
			 * The ID of this vertex.
			 */
			int id;

			/**
			 * The position of this vertex.
			 */
			Point p;

			/**
			 * A list containing the incident edges of this vertex.
			 */
			std::vector<int> incidentEdges;
		};

		/**
		 * Creates an empty graph.
		 */
		NetworkGraph();

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
		 *
		 * \param p The position of the vertex.
		 * \return The ID of the new vertex.
		 */
		int addVertex(Point p);

		/**
		 * Returns the `i`th edge in the graph.
		 *
		 * \param i The index.
		 * \return The edge.
		 */
		const Edge& edge(int i) const;

		/**
		 * Returns the `i`th edge in the graph.
		 *
		 * \param i The index.
		 * \return The edge.
		 */
		Edge& edge(int i);

		/**
		 * Returns the number of edges in the graph.
		 * \return The number of edges.
		 */
		int edgeCount() const;

		/**
		 * Adds a new edge.
		 *
		 * \param from The ID of the origin vertex.
		 * \param to The ID of the destination vertex.
		 * \param path A list of points on the edge.
		 * \param delta The δ-value.
		 * \return The ID of the new edge.
		 */
		int addEdge(int from, int to, std::vector<Point> path,
		            double delta = 0);

		/**
		 * Removes all edges that have a too low delta value.
		 *
		 * \param threshold The threshold value to use. Every edge with a delta
		 * value lower than this will be removed.
		 */
		void filterOnDelta(double threshold);

	private:

		/**
		 * List of the vertices.
		 */
		std::vector<Vertex> m_verts;

		/**
		 * List of the edges.
		 */
		std::vector<Edge> m_edges;
};

#endif // NETWORKGRAPH_H
